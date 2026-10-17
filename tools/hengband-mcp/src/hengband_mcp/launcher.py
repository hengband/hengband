"""ゲームのプロセスを、ユーザーが普段遊ぶ環境から切り離したインスタンスで起動する。

Windows 版は実行ファイルのあるディレクトリの lib を使い、セーブやスコア、設定 (Hengband.INI) を
そこへ書き込む。リポジトリの lib をそのまま使うと手元で遊んでいるゲームのデータを汚すため、
インスタンスのディレクトリへ実行ファイルと lib の読み込み専用の部分を複製して起動する。
Unix 系は -d<what>=<path> で書き込み先を差し替えられるため、複製せずにリポジトリの lib を読ませる。
"""

import logging
import os
import shutil
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Any, BinaryIO

from .client import ControlClient, ControlConnectionError, ControlRequestError

logger = logging.getLogger(__name__)

IS_WINDOWS = sys.platform == "win32"

#: lib のうちゲームが読むだけのディレクトリ。Windows ではインスタンスへ複製する
SHARED_LIB_DIRS = ("edit", "file", "help", "pref", "xtra")

#: lib のうちゲームが書き込むディレクトリ。インスタンスごとに持つ
#: (Unix 系で -d に渡す文字と対応させる)
PRIVATE_LIB_DIRS = {
    "apex": "a",
    "bone": "b",
    "data": "d",
    "info": "i",
    "save": "s",
    "script": "z",
    "user": "u",
}

#: 複製しないファイル (scores.raw はインスタンスごとのスコア)
SKIPPED_FILES = frozenset({"Makefile.am", "delete.me", "scores.raw"})

#: セーブファイルの一覧から除くファイルの拡張子 (セーブ中の一時ファイル等)
SKIPPED_SAVE_SUFFIXES = frozenset({".new", ".old", ".lok", ".tmp"})

#: ゲームの標準出力と標準エラー出力の書き出し先
LOG_NAME = "game.log"

#: 起動してから制御サーバが応答するまで待つ秒数
DEFAULT_START_TIMEOUT = 60.0

#: オプションを書く設定ファイル。pref.prf はセーブファイルより前に読まれ、セーブファイルに
#: 記録されたオプションで上書きされるため、セーブファイルの後に読まれる user.prf に書く
USER_PREF_FILE = "user.prf"

#: USER_PREF_FILE の先頭に書く注意書き
PREF_HEADER = "# hengband-mcp rewrites this file every time it starts a game.\n"

#: インスタンスで無効にするオプション。send_score はスコアの送信、view_unsafe_grids は
#: 罠を感知していない未知の場所を x で埋める表示 (地図の未知の場所が読みにくくなる)、
#: rogue_like_commands はローグ風のキー配置 (instructions はオリジナルのキー配置で説明している)
DISABLED_OPTIONS = ("send_score", "view_unsafe_grids", "rogue_like_commands")

#: Windows 版が多重起動を防ぐのに使うミューテックスの名前 (VARIANT_NAME)
WINDOWS_MUTEX_NAME = "Hengband"


class LaunchError(Exception):
    """ゲームを起動できない。"""


@dataclass(frozen=True)
class LaunchOptions:
    """ゲームの起動方法。

    :param save: 続きから遊ぶセーブファイルの名前。None なら新しいキャラクターを作る
    :param visible: ウィンドウを表示するか (Windows のみ)。False ならヘッドレスで動かす
    :param seed: 乱数の種を固定する場合の値
    :param allow_debug: デバッグコマンド (^A) を使えるようにするか
    """

    save: str | None = None
    visible: bool = False
    seed: int | None = None
    allow_debug: bool = False


class GameProcess:
    """起動したゲームのプロセス。"""

    def __init__(
        self,
        popen: subprocess.Popen[bytes],
        log_path: Path,
        log_file: BinaryIO,
        job: "WindowsJob | None",
    ) -> None:
        """
        :param popen: ゲームのプロセス
        :param log_path: ゲームの出力の書き出し先
        :param log_file: 書き出し先のファイル (プロセスが終わったら閉じる)
        :param job: ゲームを割り当てた Windows のジョブ
        """
        self.popen = popen
        self.log_path = log_path
        self._log_file = log_file
        self._job = job

    @property
    def pid(self) -> int:
        """プロセス ID。"""
        return self.popen.pid

    @property
    def exit_code(self) -> int | None:
        """終了コード。動いている間は None。"""
        return self.popen.poll()

    def is_running(self) -> bool:
        """プロセスが動いているか。

        :return: 動いているか
        """
        return self.popen.poll() is None

    def wait(self, timeout: float) -> bool:
        """プロセスの終了を待つ。

        :param timeout: 待つ秒数
        :return: 終了したか
        """
        try:
            self.popen.wait(timeout)
        except subprocess.TimeoutExpired:
            return False

        self.close()
        return True

    def kill(self) -> None:
        """プロセスを強制終了する。"""
        if self.is_running():
            self.popen.kill()
            self.wait(5.0)

        self.close()

    def close(self) -> None:
        """プロセスが終わった後に、ログのファイルとジョブを閉じる。"""
        if self.is_running():
            return

        if not self._log_file.closed:
            self._log_file.close()

        if self._job is not None:
            self._job.close()
            self._job = None

    def log_tail(self, lines: int = 20) -> str:
        """ゲームの出力の末尾を返す。

        :param lines: 返す行数
        :return: 出力の末尾
        """
        try:
            data = self.log_path.read_bytes()
        except OSError:
            return ""

        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError:
            # 日本語版のメッセージはゲームの文字コードで出力される
            text = data.decode("cp932" if IS_WINDOWS else "euc_jp", errors="replace")

        return "\n".join(text.splitlines()[-lines:])


@dataclass
class LaunchedGame:
    """起動して制御サーバが応答するようになったゲーム。

    :param process: ゲームのプロセス
    :param client: 制御サーバのクライアント
    :param encoding: 全角文字を送るときのゲームの文字コード。英語版は None
    :param info: 制御サーバの info op の結果
    """

    process: GameProcess
    client: ControlClient
    encoding: str | None
    info: dict[str, Any]


def default_instances_dir() -> Path:
    """インスタンスを置くディレクトリの既定値を返す。

    :return: Windows は %LOCALAPPDATA%、Unix 系は XDG_STATE_HOME の下の hengband-mcp/instances
    """
    if IS_WINDOWS:
        base = os.environ.get("LOCALAPPDATA")
        root = Path(base) if base else Path.home() / "AppData" / "Local"
    else:
        base = os.environ.get("XDG_STATE_HOME")
        root = Path(base) if base else Path.home() / ".local" / "state"

    return root / "hengband-mcp" / "instances"


def find_game(explicit: str | None = None) -> Path:
    """ゲームの実行ファイルを探す。

    :param explicit: 指定された実行ファイル。None なら環境変数 HENGBAND_GAME、
        それも無ければこのパッケージを含むリポジトリでビルドしたもの
    :return: 実行ファイル
    """
    candidate = explicit or os.environ.get("HENGBAND_GAME")
    if candidate:
        path = Path(candidate).expanduser().resolve()
    else:
        repository = Path(__file__).resolve().parents[4]
        path = repository / ("Hengband.exe" if IS_WINDOWS else "src/hengband")

    if not path.is_file():
        raise LaunchError(
            f"the game executable was not found at {path}. Build the game first, "
            "or pass --game (or set HENGBAND_GAME) to the hengband-mcp server"
        )

    return path


def find_lib(game: Path) -> Path:
    """ゲームの lib ディレクトリを探す。

    :param game: ゲームの実行ファイル
    :return: lib ディレクトリ
    """
    candidates = [game.parent / "lib", game.parent.parent / "lib"]
    for candidate in candidates:
        if (candidate / "edit").is_dir():
            return candidate

    raise LaunchError(f"cannot find the lib directory of the game next to {game}")


def is_same_file(source: Path, destination: Path) -> bool:
    """複製先が複製元と同じ内容とみなせるか (大きさと更新日時が同じか)。

    :param source: 複製元
    :param destination: 複製先
    :return: 同じとみなせるか
    """
    try:
        source_stat = source.stat()
        destination_stat = destination.stat()
    except OSError:
        return False

    return (
        source_stat.st_size == destination_stat.st_size
        and source_stat.st_mtime_ns == destination_stat.st_mtime_ns
    )


def copy_if_changed(source: Path, destination: Path) -> bool:
    """大きさか更新日時が異なる場合だけファイルを複製する。

    :param source: 複製元
    :param destination: 複製先
    :return: 複製したか
    """
    if is_same_file(source, destination):
        return False

    destination.parent.mkdir(parents=True, exist_ok=True)
    try:
        shutil.copy2(source, destination)
    except PermissionError as error:
        raise LaunchError(
            f"cannot update {destination}; is a game from this instance still running? ({error})"
        ) from error

    return True


def mirror_tree(source: Path, destination: Path) -> int:
    """ディレクトリの下のファイルを、変わったものだけ複製する。

    :param source: 複製元のディレクトリ
    :param destination: 複製先のディレクトリ
    :return: 複製したファイルの数
    """
    copied = 0
    for root, _, files in os.walk(source):
        relative = Path(root).relative_to(source)
        for name in files:
            if name in SKIPPED_FILES:
                continue

            if copy_if_changed(Path(root) / name, destination / relative / name):
                copied += 1

    return copied


def prepare_instance(
    instance_dir: Path, game: Path, lib: Path, allow_debug: bool
) -> Path:
    """インスタンスのディレクトリを用意する。

    :param instance_dir: インスタンスのディレクトリ
    :param game: ゲームの実行ファイル
    :param lib: ゲームの lib ディレクトリ
    :param allow_debug: デバッグコマンドを使えるようにするか
    :return: 起動する実行ファイル
    """
    instance_lib = instance_dir / "lib"
    for name in PRIVATE_LIB_DIRS:
        (instance_lib / name).mkdir(parents=True, exist_ok=True)

    executable = game
    if IS_WINDOWS:
        executable = instance_dir / game.name
        copied = int(copy_if_changed(game, executable))
        for name in SHARED_LIB_DIRS:
            if (lib / name).is_dir():
                copied += mirror_tree(lib / name, instance_lib / name)

        if copied:
            logger.info("updated %d files in %s", copied, instance_dir)

    # 既存のスコア (h_scores.raw 等) は無ければ写す。インスタンスのスコアは上書きしない
    apex = lib / "apex"
    if apex.is_dir():
        for source in apex.iterdir():
            destination = instance_lib / "apex" / source.name
            if source.is_file() and source.name not in SKIPPED_FILES and not destination.exists():
                shutil.copy2(source, destination)

    lines = [PREF_HEADER, *(f"X:{name}\n" for name in DISABLED_OPTIONS)]
    # デバッグを許可して遊んだセーブファイルでも、許可しないときは明示して無効にする
    lines.append(f"{'Y' if allow_debug else 'X'}:allow_debug_opts\n")
    (instance_lib / "user" / USER_PREF_FILE).write_text("".join(lines), encoding="ascii")
    return executable


def find_free_port() -> int:
    """空いているポートを探す。

    :return: ポート番号
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        return int(probe.getsockname()[1])


def save_path(instance_dir: Path, name: str) -> Path:
    """セーブファイルの名前から、インスタンスの中のパスを求める。

    :param instance_dir: インスタンスのディレクトリ
    :param name: list_saves() が返す名前
    :return: セーブファイルのパス (Unix 系では見つからなければ名前どおりのパス)
    """
    save_dir = instance_dir / "lib" / "save"
    if Path(name).name != name or name in ("", ".", ".."):
        raise LaunchError(f"invalid save name: {name!r}")

    path = save_dir / name
    if not IS_WINDOWS and not path.is_file():
        # Unix 系のセーブファイルは「<ユーザー ID>.<名前>」の場合がある
        prefixed = save_dir / f"{os.getuid()}.{name}"
        if prefixed.is_file():
            return prefixed

    return path


def list_saves(instance_dir: Path) -> list[dict[str, Any]]:
    """インスタンスのセーブファイルを一覧にする。

    :param instance_dir: インスタンスのディレクトリ
    :return: セーブファイルの名前、更新日時、大きさの一覧 (新しい順)
    """
    save_dir = instance_dir / "lib" / "save"
    if not save_dir.is_dir():
        return []

    saves = []
    for path in save_dir.iterdir():
        if not path.is_file() or path.name in SKIPPED_FILES:
            continue

        if path.suffix.lower() in SKIPPED_SAVE_SUFFIXES:
            continue

        name = path.name
        prefix, separator, rest = name.partition(".")
        if not IS_WINDOWS and separator and prefix.isdigit() and rest:
            name = rest

        stat = path.stat()
        saves.append(
            {
                "name": name,
                "modified": datetime.fromtimestamp(stat.st_mtime).isoformat(
                    sep=" ", timespec="seconds"
                ),
                "size": stat.st_size,
            }
        )

    saves.sort(key=lambda save: save["modified"], reverse=True)
    return saves


def build_command(
    executable: Path,
    instance_dir: Path,
    port: int,
    options: LaunchOptions,
) -> list[str]:
    """ゲームのコマンドラインを組み立てる。

    :param executable: 起動する実行ファイル
    :param instance_dir: インスタンスのディレクトリ
    :param port: 制御サーバのポート
    :param options: 起動方法
    :return: コマンドライン
    """
    command = [str(executable)]
    if not options.visible:
        command.append("--headless")

    command.append(f"--control-port={port}")
    if options.seed is not None:
        command.append(f"--fixed-seed={options.seed}")

    if not IS_WINDOWS:
        instance_lib = instance_dir / "lib"
        for name, letter in PRIVATE_LIB_DIRS.items():
            command.append(f"-d{letter}={instance_lib / name}")

    if options.save is None:
        # Windows のウィンドウ版は -n が無いと [ファイル] メニューの操作を待つ
        command.append("-n")
    elif IS_WINDOWS:
        command.append(str(save_path(instance_dir, options.save)))
    else:
        command.append(f"-u{options.save}")

    return command


def is_windows_game_running() -> bool:
    """Windows で変愚蛮怒が動いているか (多重起動防止のミューテックスがあるか) を調べる。

    :return: 動いているか。調べられない場合は False
    """
    if not IS_WINDOWS:
        return False

    import ctypes
    from ctypes import wintypes

    synchronize = 0x00100000
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel32.OpenMutexW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
    kernel32.OpenMutexW.restype = wintypes.HANDLE
    kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
    handle = kernel32.OpenMutexW(synchronize, False, WINDOWS_MUTEX_NAME)
    if not handle:
        return False

    kernel32.CloseHandle(handle)
    return True


def launch(
    game: Path,
    instance_dir: Path,
    options: LaunchOptions,
    timeout: float = DEFAULT_START_TIMEOUT,
) -> LaunchedGame:
    """ゲームを起動し、制御サーバが応答するまで待つ。

    :param game: ゲームの実行ファイル
    :param instance_dir: インスタンスのディレクトリ
    :param options: 起動方法
    :param timeout: 制御サーバが応答するまで待つ秒数
    :return: 起動したゲーム
    """
    if options.visible and not IS_WINDOWS:
        raise LaunchError(
            "display='window' is supported only on Windows; on other systems the game always runs headless"
        )

    if is_windows_game_running():
        raise LaunchError(
            "Hengband is already running on this PC, and the Windows version can run only one game "
            "at a time. Save and close the other game, then try again"
        )

    lib = find_lib(game)
    instance_dir.mkdir(parents=True, exist_ok=True)
    if options.save is not None and not save_path(instance_dir, options.save).is_file():
        raise LaunchError(
            f"no save named {options.save!r} in this instance; call list_saves to see the saves"
        )

    executable = prepare_instance(instance_dir, game, lib, options.allow_debug)
    port = find_free_port()
    command = build_command(executable, instance_dir, port, options)
    environment = dict(os.environ)
    if not IS_WINDOWS:
        environment["ANGBAND_PATH"] = str(lib) + os.sep

    log_path = instance_dir / LOG_NAME
    log_file = open(log_path, "wb")
    logger.info("starting %s", subprocess.list2cmdline(command))
    try:
        # 標準入出力を引き継ぐと MCP の通信 (stdio) を壊すため、すべて差し替える
        popen = subprocess.Popen(
            command,
            cwd=instance_dir,
            env=environment,
            stdin=subprocess.DEVNULL,
            stdout=log_file,
            stderr=subprocess.STDOUT,
        )
    except OSError as error:
        log_file.close()
        raise LaunchError(f"cannot start {executable}: {error}") from error

    job = None
    if IS_WINDOWS and not options.visible:
        # このサーバが異常終了しても、ヘッドレスのゲームが残り続けないようにする
        job = WindowsJob.create_for(popen.pid)

    process = GameProcess(popen, log_path, log_file, job)
    client = ControlClient(port)
    try:
        info = wait_until_ready(process, client, timeout)
    except BaseException:
        process.kill()
        raise

    encoding = None
    if info.get("japanese"):
        encoding = "cp932" if IS_WINDOWS else "euc_jp"

    return LaunchedGame(process, client, encoding, info)


def wait_until_ready(
    process: GameProcess, client: ControlClient, timeout: float
) -> dict[str, Any]:
    """制御サーバが応答するまで待つ。

    :param process: ゲームのプロセス
    :param client: 制御サーバのクライアント
    :param timeout: 待つ秒数
    :return: info op の結果
    """
    deadline = time.monotonic() + timeout
    while True:
        if not process.is_running():
            log = process.log_tail()
            hint = ""
            if "already running" in log:
                hint = " Another Hengband is running; the Windows version can run only one game at a time."

            raise LaunchError(
                f"the game exited during startup (exit code {process.exit_code}).{hint}\n{log}"
            )

        left = deadline - time.monotonic()
        if left <= 0:
            raise LaunchError(
                f"the game did not become ready within {timeout:g} seconds\n{process.log_tail()}"
            )

        try:
            return client.request("info", timeout=min(5.0, max(0.5, left)))
        except ControlConnectionError:
            time.sleep(0.1)
        except ControlRequestError as error:
            raise LaunchError(f"the game rejected the info request: {error}") from error


class WindowsJob:
    """プロセスを割り当てた Windows のジョブ。

    ジョブのハンドルを閉じると (このサーバが終了したときも OS が閉じる)、割り当てたプロセスも終了する。
    """

    def __init__(self, handle: int) -> None:
        """
        :param handle: ジョブのハンドル
        """
        self._handle = handle

    @classmethod
    def create_for(cls, pid: int) -> "WindowsJob | None":
        """ジョブを作り、プロセスを割り当てる。

        :param pid: 割り当てるプロセス
        :return: ジョブ。作れなかった場合は None
        """
        import ctypes
        from ctypes import wintypes

        class IoCounters(ctypes.Structure):
            _fields_ = [
                ("ReadOperationCount", ctypes.c_ulonglong),
                ("WriteOperationCount", ctypes.c_ulonglong),
                ("OtherOperationCount", ctypes.c_ulonglong),
                ("ReadTransferCount", ctypes.c_ulonglong),
                ("WriteTransferCount", ctypes.c_ulonglong),
                ("OtherTransferCount", ctypes.c_ulonglong),
            ]

        class BasicLimitInformation(ctypes.Structure):
            _fields_ = [
                ("PerProcessUserTimeLimit", ctypes.c_int64),
                ("PerJobUserTimeLimit", ctypes.c_int64),
                ("LimitFlags", wintypes.DWORD),
                ("MinimumWorkingSetSize", ctypes.c_size_t),
                ("MaximumWorkingSetSize", ctypes.c_size_t),
                ("ActiveProcessLimit", wintypes.DWORD),
                ("Affinity", ctypes.c_size_t),
                ("PriorityClass", wintypes.DWORD),
                ("SchedulingClass", wintypes.DWORD),
            ]

        class ExtendedLimitInformation(ctypes.Structure):
            _fields_ = [
                ("BasicLimitInformation", BasicLimitInformation),
                ("IoInfo", IoCounters),
                ("ProcessMemoryLimit", ctypes.c_size_t),
                ("JobMemoryLimit", ctypes.c_size_t),
                ("PeakProcessMemoryUsed", ctypes.c_size_t),
                ("PeakJobMemoryUsed", ctypes.c_size_t),
            ]

        job_object_extended_limit_information = 9
        job_object_limit_kill_on_job_close = 0x2000
        process_set_quota = 0x0100
        process_terminate = 0x0001

        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.CreateJobObjectW.argtypes = [wintypes.LPVOID, wintypes.LPCWSTR]
        kernel32.CreateJobObjectW.restype = wintypes.HANDLE
        kernel32.SetInformationJobObject.argtypes = [
            wintypes.HANDLE,
            ctypes.c_int,
            wintypes.LPVOID,
            wintypes.DWORD,
        ]
        kernel32.SetInformationJobObject.restype = wintypes.BOOL
        kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
        kernel32.OpenProcess.restype = wintypes.HANDLE
        kernel32.AssignProcessToJobObject.argtypes = [wintypes.HANDLE, wintypes.HANDLE]
        kernel32.AssignProcessToJobObject.restype = wintypes.BOOL
        kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
        kernel32.CloseHandle.restype = wintypes.BOOL

        job = kernel32.CreateJobObjectW(None, None)
        if not job:
            logger.warning("CreateJobObjectW failed: %d", ctypes.get_last_error())
            return None

        information = ExtendedLimitInformation()
        information.BasicLimitInformation.LimitFlags = job_object_limit_kill_on_job_close
        process = None
        try:
            if not kernel32.SetInformationJobObject(
                job,
                job_object_extended_limit_information,
                ctypes.byref(information),
                ctypes.sizeof(information),
            ):
                raise OSError(ctypes.get_last_error(), "SetInformationJobObject failed")

            process = kernel32.OpenProcess(process_set_quota | process_terminate, False, pid)
            if not process:
                raise OSError(ctypes.get_last_error(), "OpenProcess failed")

            if not kernel32.AssignProcessToJobObject(job, process):
                raise OSError(ctypes.get_last_error(), "AssignProcessToJobObject failed")
        except OSError as error:
            logger.warning("cannot tie the game to this server: %s", error)
            kernel32.CloseHandle(job)
            return None
        finally:
            if process:
                kernel32.CloseHandle(process)

        return cls(job)

    def close(self) -> None:
        """ジョブを閉じる。割り当てたプロセスがまだ動いていれば終了する。"""
        if not self._handle:
            return

        import ctypes
        from ctypes import wintypes

        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
        kernel32.CloseHandle(self._handle)
        self._handle = 0
