import os
import subprocess
import sys
from pathlib import Path

import pytest

from hengband_mcp import launcher
from hengband_mcp.client import ControlClient
from hengband_mcp.launcher import (
    PRIVATE_LIB_DIRS,
    LaunchError,
    LaunchOptions,
    WindowsJob,
    build_command,
    find_game,
    find_lib,
    launch,
    list_saves,
    mirror_tree,
    prepare_instance,
    save_path,
    wait_until_ready,
)


def write(path: Path, text: str = "x") -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
    return path


def touch(path: Path, timestamp: int) -> None:
    os.utime(path, (timestamp, timestamp))


@pytest.fixture
def game(tmp_path: Path) -> Path:
    """ビルドしたリポジトリと同じ配置の実行ファイルと lib を作る。"""
    root = tmp_path / "repository"
    executable = write(root / "Hengband.exe", "exe")
    write(root / "lib" / "edit" / "MonraceDefinitions.jsonc", "{}")
    write(root / "lib" / "edit" / "Makefile.am", "")
    write(root / "lib" / "help" / "general.txt", "help")
    write(root / "lib" / "apex" / "h_scores.raw", "scores")
    write(root / "lib" / "apex" / "scores.raw", "scores of the user")
    write(root / "lib" / "save" / "delete.me", "")
    return executable


@pytest.fixture
def windows(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(launcher, "IS_WINDOWS", True)


@pytest.fixture
def unix(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(launcher, "IS_WINDOWS", False)


def test_find_lib_next_to_the_game(game: Path) -> None:
    assert find_lib(game) == game.parent / "lib"


def test_find_lib_above_the_game(tmp_path: Path) -> None:
    # Unix 系の src/hengband はリポジトリの lib を使う
    executable = write(tmp_path / "src" / "hengband")
    write(tmp_path / "lib" / "edit" / "TerrainDefinitions.jsonc")

    assert find_lib(executable) == tmp_path / "lib"


def test_find_lib_reports_a_missing_lib(tmp_path: Path) -> None:
    with pytest.raises(LaunchError, match="lib directory"):
        find_lib(write(tmp_path / "hengband"))


def test_find_game_uses_the_given_path(game: Path) -> None:
    assert find_game(str(game)) == game.resolve()


def test_find_game_uses_the_environment(game: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("HENGBAND_GAME", str(game))

    assert find_game() == game.resolve()


def test_find_game_reports_a_missing_executable(tmp_path: Path) -> None:
    with pytest.raises(LaunchError, match="not found"):
        find_game(str(tmp_path / "missing.exe"))


def test_mirror_tree_copies_only_changed_files(tmp_path: Path) -> None:
    source = tmp_path / "source"
    destination = tmp_path / "destination"
    write(source / "a.txt", "a")
    write(source / "sub" / "b.txt", "b")
    write(source / "Makefile.am", "")

    assert mirror_tree(source, destination) == 2
    assert (destination / "sub" / "b.txt").read_text() == "b"
    assert not (destination / "Makefile.am").exists()
    assert mirror_tree(source, destination) == 0

    write(source / "a.txt", "changed")
    touch(source / "a.txt", 2_000_000_000)
    assert mirror_tree(source, destination) == 1
    assert (destination / "a.txt").read_text() == "changed"


def test_prepare_instance_on_windows_copies_the_game(
    game: Path, tmp_path: Path, windows: None
) -> None:
    instance = tmp_path / "instance"
    executable = prepare_instance(instance, game, game.parent / "lib", allow_debug=False)

    assert executable == instance / "Hengband.exe"
    assert executable.read_text() == "exe"
    assert (instance / "lib" / "edit" / "MonraceDefinitions.jsonc").is_file()
    assert (instance / "lib" / "help" / "general.txt").is_file()
    assert not (instance / "lib" / "edit" / "Makefile.am").exists()
    for name in PRIVATE_LIB_DIRS:
        assert (instance / "lib" / name).is_dir()

    assert (instance / "lib" / "apex" / "h_scores.raw").read_text() == "scores"
    assert not (instance / "lib" / "apex" / "scores.raw").exists()
    assert not (instance / "lib" / "save" / "delete.me").exists()
    pref = (instance / "lib" / "user" / "user.prf").read_text()
    assert "X:send_score" in pref
    assert "X:view_unsafe_grids" in pref
    assert "X:rogue_like_commands" in pref
    assert "X:allow_debug_opts" in pref


def test_prepare_instance_keeps_the_scores_of_the_instance(
    game: Path, tmp_path: Path, windows: None
) -> None:
    instance = tmp_path / "instance"
    write(instance / "lib" / "apex" / "h_scores.raw", "scores of the instance")
    prepare_instance(instance, game, game.parent / "lib", allow_debug=False)

    assert (instance / "lib" / "apex" / "h_scores.raw").read_text() == "scores of the instance"


def test_prepare_instance_allows_debug_commands(game: Path, tmp_path: Path) -> None:
    instance = tmp_path / "instance"
    pref = instance / "lib" / "user" / "user.prf"
    prepare_instance(instance, game, game.parent / "lib", allow_debug=True)
    prepare_instance(instance, game, game.parent / "lib", allow_debug=False)
    assert "Y:allow_debug_opts" not in pref.read_text()

    prepare_instance(instance, game, game.parent / "lib", allow_debug=True)
    assert "Y:allow_debug_opts" in pref.read_text()
    assert "X:allow_debug_opts" not in pref.read_text()


def test_prepare_instance_on_unix_runs_the_game_in_place(
    game: Path, tmp_path: Path, unix: None
) -> None:
    instance = tmp_path / "instance"
    executable = prepare_instance(instance, game, game.parent / "lib", allow_debug=False)

    assert executable == game
    assert not (instance / "lib" / "edit").exists()
    assert (instance / "lib" / "user" / "user.prf").is_file()


@pytest.mark.parametrize("name", ["", ".", "..", "../Alice", "save/Alice"])
def test_save_path_rejects_paths(tmp_path: Path, name: str) -> None:
    with pytest.raises(LaunchError, match="invalid save name"):
        save_path(tmp_path, name)


def test_list_saves_lists_the_newest_first(tmp_path: Path, windows: None) -> None:
    save_dir = tmp_path / "lib" / "save"
    touch(write(save_dir / "Alice", "1234"), 1_600_000_000)
    touch(write(save_dir / "Bob"), 1_700_000_000)
    for name in ("Bob.new", "Bob.old", "Carol.lok", "delete.me"):
        write(save_dir / name)

    saves = list_saves(tmp_path)

    assert [save["name"] for save in saves] == ["Bob", "Alice"]
    assert saves[1]["size"] == 4
    assert saves[1]["modified"].startswith("2020-09-")


def test_list_saves_hides_the_user_id_on_unix(tmp_path: Path, unix: None) -> None:
    write(tmp_path / "lib" / "save" / "1000.Alice")

    assert [save["name"] for save in list_saves(tmp_path)] == ["Alice"]


def test_list_saves_without_saves(tmp_path: Path) -> None:
    assert list_saves(tmp_path) == []


def test_build_command_for_a_new_game(tmp_path: Path, windows: None) -> None:
    executable = tmp_path / "Hengband.exe"

    assert build_command(executable, tmp_path, 9100, LaunchOptions()) == [
        str(executable),
        "--headless",
        "--control-port=9100",
        "-n",
    ]


def test_build_command_for_a_saved_game_in_a_window(tmp_path: Path, windows: None) -> None:
    executable = tmp_path / "Hengband.exe"
    options = LaunchOptions(save="Alice", visible=True, seed=42)

    assert build_command(executable, tmp_path, 9100, options) == [
        str(executable),
        "--control-port=9100",
        "--fixed-seed=42",
        str(tmp_path / "lib" / "save" / "Alice"),
    ]


def test_build_command_on_unix_redirects_the_writable_directories(
    tmp_path: Path, unix: None
) -> None:
    command = build_command(Path("hengband"), tmp_path, 9100, LaunchOptions(save="Alice"))

    assert command[:3] == ["hengband", "--headless", "--control-port=9100"]
    assert f"-ds={tmp_path / 'lib' / 'save'}" in command
    assert f"-du={tmp_path / 'lib' / 'user'}" in command
    assert command[-1] == "-uAlice"


def test_launch_shows_a_window_only_on_windows(game: Path, tmp_path: Path, unix: None) -> None:
    with pytest.raises(LaunchError, match="only on Windows"):
        launch(game, tmp_path / "instance", LaunchOptions(visible=True))


def test_launch_refuses_while_another_game_runs(
    game: Path, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(launcher, "is_windows_game_running", lambda: True)

    with pytest.raises(LaunchError, match="already running"):
        launch(game, tmp_path / "instance", LaunchOptions())


def test_launch_reports_a_missing_save(
    game: Path, tmp_path: Path, monkeypatch: pytest.MonkeyPatch, windows: None
) -> None:
    monkeypatch.setattr(launcher, "is_windows_game_running", lambda: False)

    with pytest.raises(LaunchError, match="list_saves"):
        launch(game, tmp_path / "instance", LaunchOptions(save="Nobody"))


class ExitedProcess:
    exit_code = 1

    def is_running(self) -> bool:
        return False

    def log_tail(self, lines: int = 20) -> str:
        return "Hengband is already running."


def test_wait_until_ready_reports_a_game_that_exited() -> None:
    with pytest.raises(LaunchError, match="exit code 1") as error:
        wait_until_ready(ExitedProcess(), ControlClient(1), 5.0)  # type: ignore[arg-type]

    assert "only one game" in str(error.value)
    assert "Hengband is already running." in str(error.value)


@pytest.mark.skipif(sys.platform != "win32", reason="Windows only")
def test_windows_job_ends_the_game_when_closed() -> None:
    child = subprocess.Popen(
        ["ping", "-n", "30", "127.0.0.1"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    try:
        job = WindowsJob.create_for(child.pid)
        assert job is not None
        job.close()
        child.wait(10)
    finally:
        if child.poll() is None:
            child.kill()
