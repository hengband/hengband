"""1 つのゲームとのやり取り (キーの送信、-more- の処理、観測、終了) を受け持つ。"""

import time
from dataclasses import dataclass
from typing import Any, Protocol

from .client import (
    ControlClient,
    ControlConnectionError,
    ControlRequestError,
    ControlTimeoutError,
)
from .keys import Key, encode_byte, join_text, parse_keys
from .observation import (
    InputState,
    View,
    build_observation,
    render_screen,
    screen_lines,
)

ESCAPE = encode_byte(0x1B)

#: セーブして終了するコマンド (^X)
SAVE_AND_QUIT = encode_byte(0x18)

#: キー 1 つの後に閉じる -more- の上限 (閉じても閉じても現れる場合に止まらなくなるのを防ぐ)
MAX_MORE_PER_KEY = 100

#: プロンプトやメニューを抜けてコマンド入力待ちに戻すために送る ESC の上限
MAX_ESCAPES = 10

#: 死亡後の画面を抜けるために送る ESC の上限
MAX_DEATH_ESCAPES = 30

#: 最初の観測で返す、それまでのメッセージの数
FIRST_MESSAGES = 10

#: 1 回の観測で返すメッセージの上限
MAX_MESSAGES = 50

#: send_keys の既定の制限時間 (秒)
DEFAULT_KEYS_TIMEOUT = 60.0

#: send_keys で送らせないキーと、その理由
REFUSED_KEYS = {
    SAVE_AND_QUIT: "<C-x> saves and quits the game; call stop_game instead",
}

#: デバッグを許可して起動したときだけ送れるキーと、その理由
DEBUG_KEYS = {
    encode_byte(0x01): "<C-a> (debug commands) is disabled; start the game with allow_debug=true to use it",
    encode_byte(0x17): "<C-w> (wizard mode) is disabled; start the game with allow_debug=true to use it",
}


class StopError(Exception):
    """セーブして終了できなかった (ゲームは動いたまま)。"""


class KeyRefusedError(ValueError):
    """送らせないキーが含まれている。"""


class GameProcess(Protocol):
    """終了を待てるゲームのプロセス。"""

    def is_running(self) -> bool: ...

    def wait(self, timeout: float) -> bool: ...

    def kill(self) -> None: ...


@dataclass(frozen=True)
class KeysResult:
    """send_keys の結果。

    :param outcome: done (すべて送った)、disturbed (邪魔が入って途中で止めた)、
        timeout (制限時間を過ぎた)、game_over (キャラクターが死亡した)
    :param consumed: ゲームに送ったキー (LLM が書いた表記)。timeout の場合、最後のキーは処理されたか分からない
    :param remaining: 送らなかったキー
    :param more_dismissed: 自動で閉じた -more- の数
    :param game_turns: キーを送る間に進んだゲームターン数 (通常の速度の 1 行動で平均 10)。
        0 ならゲームの時間は進んでいない (メニューやプロンプトの操作、ゲームが受け付けなかったコマンド)。
        分からない場合 (timeout や、game_turn を返さない古いゲーム) は None
    """

    outcome: str
    consumed: str
    remaining: str
    more_dismissed: int
    game_turns: int | None = None


class GameSession:
    """制御サーバを通じて 1 つのゲームを操作する。"""

    def __init__(
        self,
        client: ControlClient,
        encoding: str | None,
        process: GameProcess | None = None,
        allow_debug: bool = False,
    ) -> None:
        """
        :param client: 制御サーバのクライアント
        :param encoding: 全角文字を送るときのゲームの文字コード。None なら英語版
        :param process: ゲームのプロセス。終了を待つのに使う
        :param allow_debug: デバッグコマンドとウィザードモードのキーを送らせるか
        """
        self.client = client
        self.encoding = encoding
        self.process = process
        self.allow_debug = allow_debug
        self.state: InputState | None = None
        self._message_sequence: int | None = None
        self._message_tail: str | None = None

    def read_input(self, timeout: float | None = None) -> InputState:
        """ゲームが待っているキー入力の情報を読む。

        ゲームが処理を続けている間は、次にキー入力を待つまで応答が返らない。

        :param timeout: 応答を待つ秒数。None ならクライアントの既定値
        :return: 入力待ちの情報
        """
        self.state = None
        self.state = InputState.from_result(
            self.client.request("input", timeout=timeout)
        )
        return self.state

    def dismiss_more(self, deadline: float) -> int:
        """-more- が出ていれば ESC で閉じる。

        ESC で閉じると、そのターンの残りのメッセージでは -more- が出なくなる。
        メッセージは履歴に残り、観測の messages で返すので読み落とすことはない。
        死亡後の -more- は、最後のメッセージを画面に残すため閉じない。

        :param deadline: 打ち切る時刻 (time.monotonic() の値)
        :return: 閉じた -more- の数
        """
        dismissed = 0
        state = self.read_input(remaining(deadline))
        while state.kind == "more" and not state.dead:
            if dismissed >= MAX_MORE_PER_KEY or time.monotonic() > deadline:
                break

            self.client.request("keys", keys=ESCAPE)
            dismissed += 1
            state = self.read_input(remaining(deadline))

        return dismissed

    def send_keys(
        self,
        text: str,
        stop_on_disturb: bool = True,
        timeout: float = DEFAULT_KEYS_TIMEOUT,
    ) -> KeysResult:
        """キーを 1 つずつ送る。

        キーを送るたびに、ゲームが次の入力を待つまで処理させてから状態を読む。
        邪魔 (disturb) が入ってコマンド入力待ちに戻った場合は、残りのキーを送らずに止める。
        制限時間を過ぎてもゲームが処理を続けている場合は timeout を返す。

        :param text: LLM が書いたキー列
        :param stop_on_disturb: 邪魔が入ったら残りのキーを送らずに止めるか
        :param timeout: 制限時間 (秒)
        :return: 送った結果
        """
        keys = parse_keys(text, self.encoding)
        self.check_refused(keys)
        deadline = time.monotonic() + timeout
        outcome = "done"
        sent = 0
        dismissed = 0
        start_turn = None
        try:
            dismissed += self.dismiss_more(deadline)
            start_turn = self._current_state().game_turn
            for key in keys:
                state = self._current_state()
                if state.dead:
                    outcome = "game_over"
                    break

                if time.monotonic() > deadline:
                    outcome = "timeout"
                    break

                baseline = state.disturb_sequence
                sent += 1
                self.client.request("keys", keys=key.wire)
                dismissed += self.dismiss_more(deadline)
                state = self._current_state()
                if state.dead:
                    outcome = "game_over"
                    break

                disturbed = state.disturb_sequence != baseline
                if stop_on_disturb and disturbed and state.kind == "command":
                    if sent < len(keys):
                        outcome = "disturbed"
                        break
        except ControlTimeoutError:
            # ゲームはまだ処理を続けている。次の観測で入力待ちの情報を読み直す
            outcome = "timeout"
            self.state = None

        if outcome == "done" and self._current_state().dead:
            outcome = "game_over"

        game_turns = None
        end_turn = self.state.game_turn if self.state is not None else None
        if start_turn is not None and end_turn is not None:
            game_turns = end_turn - start_turn

        return KeysResult(
            outcome=outcome,
            consumed=join_text(keys[:sent]),
            remaining=join_text(keys[sent:]),
            more_dismissed=dismissed,
            game_turns=game_turns,
        )

    def check_refused(self, keys: list[Key]) -> None:
        """送らせないキーが含まれていれば、1 つも送らずに KeyRefusedError を送出する。

        :param keys: 送るキーの並び
        """
        for key in keys:
            reason = REFUSED_KEYS.get(key.wire)
            if reason is None and not self.allow_debug:
                reason = DEBUG_KEYS.get(key.wire)

            if reason is not None:
                raise KeyRefusedError(f"refused to send any keys: {reason}")

    def observe(
        self, view: View = "auto", include_items: bool = False, refresh: bool = True
    ) -> tuple[dict[str, Any], str | None]:
        """現在の状態を観測する。

        :param view: 観測に含める画面の範囲
        :param include_items: 持ち物と装備を含めるか
        :param refresh: 入力待ちの情報を読み直すか (キーを送った直後なら不要)
        :return: JSON にする要約と、画面のテキスト
        """
        state = self.read_input() if refresh or self.state is None else self.state
        screen = None
        if view != "none" or state.kind != "command":
            screen = self.client.request("screen", attrs=False)

        snapshot = None
        if state.character_generated:
            try:
                snapshot = self.client.request("state", map=False)
            except ControlRequestError:
                # キャラクター生成の直後など、内部状態をまだ組み立てられない
                snapshot = None

        messages, omitted = self.new_messages(state.message_sequence)
        return build_observation(
            state,
            screen,
            snapshot,
            messages,
            omitted,
            view,
            self.encoding,
            include_items,
        )

    def screen_text(self) -> str:
        """画面全体をテキストで返す。

        :return: 画面の各行を改行でつないだ文字列
        """
        return render_screen(screen_lines(self.client.request("screen", attrs=False)))

    def new_messages(self, sequence: int) -> tuple[list[str], int]:
        """前回の観測以降に増えたメッセージを古い順に返す。

        message_sequence は履歴に行が加わるたびに増える。同じメッセージが続いた場合は
        行が増えずに最新行の「<xN>」だけが変わるので、最新行の文字列の変化でも検出する。
        新規ゲームの開始時に履歴の区切りとして加わる空白だけの行は返さない。

        :param sequence: 現在の message_sequence
        :return: 増えたメッセージと、上限を超えて省いた数
        """
        if self._message_sequence is None:
            fetched = self._fetch_messages(FIRST_MESSAGES)
            self._remember_messages(sequence, fetched)
            return without_blanks(fetched), 0

        added = max(0, sequence - self._message_sequence)
        shown = min(added, MAX_MESSAGES)
        fetched = self._fetch_messages(shown + 1)
        split = max(0, len(fetched) - shown)
        fresh = fetched[split:]
        older = fetched[:split]
        if shown == added and older and older[-1] != self._message_tail:
            # 前回の最新行が繰り返されて「<xN>」が増えてから、新しい行が加わった
            fresh.insert(0, older[-1])

        self._remember_messages(sequence, fetched)
        return without_blanks(fresh), added - shown

    def _fetch_messages(self, count: int) -> list[str]:
        """メッセージ履歴の新しい方から count 件を古い順に読む。

        :param count: 読む件数
        :return: メッセージの一覧
        """
        messages = self.client.request("messages", count=count).get("messages", [])
        return [str(message) for message in messages]

    def _remember_messages(self, sequence: int, fetched: list[str]) -> None:
        """次の観測で差分を求めるため、現在の履歴の位置を覚える。

        :param sequence: 現在の message_sequence
        :param fetched: 読んだメッセージ (最後が最新行)
        """
        self._message_sequence = sequence
        if fetched:
            self._message_tail = fetched[-1]

    def _current_state(self) -> InputState:
        """直前に読んだ入力待ちの情報を返す。まだ読んでいなければ読む。

        :return: 入力待ちの情報
        """
        return self.state if self.state is not None else self.read_input()

    def stop(
        self, save: bool, timeout: float = 30.0, force: bool = False
    ) -> dict[str, Any]:
        """ゲームを終了させる。

        セーブする場合は、プロンプトを抜けてコマンド入力待ちに戻してから ^X (セーブして終了) を送り、
        最後の「リターンキーか ESC キーを押して下さい」に ESC で答える (ほかのキーではスコアを表示する)。
        死亡している場合は、死亡後の画面を ESC で抜ける。それでも終わらない場合は quit op で終了させ、
        最後はプロセスを強制終了する。

        :param save: セーブしてから終了するか
        :param timeout: 制限時間 (秒)
        :param force: セーブできない場合もセーブせずに終了させるか。False なら StopError を送出し、
            ゲームは動いたままにする
        :return: 終了の結果
        """
        deadline = time.monotonic() + timeout
        result: dict[str, Any] = {"saved": False}
        try:
            state = self.read_input(remaining(deadline))
            if state.dead:
                result["dead"] = True
                self._leave_death_screens(deadline)
            elif save and state.character_generated:
                result["saved"] = self._save_and_quit(deadline, force)
            elif save:
                result["note"] = "the character was not created yet, so nothing was saved"
        except ControlConnectionError:
            # 既に終了している、または応答しない。下で quit op を送り、だめなら強制終了する
            pass

        if self._is_running():
            try:
                self.client.request("quit", timeout=5.0)
            except (ControlConnectionError, ControlRequestError):
                pass

            if self.process is not None and not self.process.wait(5.0):
                self.process.kill()
                result["killed"] = True

        return result

    def _save_and_quit(self, deadline: float, force: bool) -> bool:
        """セーブして終了させる。

        :param deadline: 打ち切る時刻
        :param force: コマンド入力待ちに戻れない場合に、セーブせずに終了させてよいか
        :return: セーブして終了できたか
        """
        state = self._current_state()
        escapes = 0
        while state.kind != "command" and escapes < MAX_ESCAPES:
            self.client.request("keys", keys=ESCAPE)
            escapes += 1
            self.dismiss_more(deadline)
            state = self._current_state()

        if state.kind != "command":
            if force:
                return False

            raise StopError(
                f"cannot get back to the command prompt to save (the game is waiting for {state.kind!r} input). "
                "The game is still running; leave the current screen with send_keys and try again, "
                "or call stop_game with save=false to quit without saving"
            )

        self.client.request("keys", keys=SAVE_AND_QUIT)
        self.dismiss_more(deadline)
        self.client.request("keys", keys=ESCAPE)
        return self._wait_exit(deadline)

    def _leave_death_screens(self, deadline: float) -> None:
        """死亡後の画面を ESC で抜けて終了させる。

        :param deadline: 打ち切る時刻
        """
        for _ in range(MAX_DEATH_ESCAPES):
            if not self._is_running() or time.monotonic() > deadline:
                return

            try:
                self.client.request("keys", keys=ESCAPE)
                self.read_input()
            except ControlConnectionError:
                self._wait_exit(deadline)
                return

    def _wait_exit(self, deadline: float) -> bool:
        """プロセスの終了を待つ。

        :param deadline: 打ち切る時刻
        :return: 終了したか。プロセスを持たない場合は True
        """
        if self.process is None:
            return True

        return self.process.wait(remaining(deadline))

    def _is_running(self) -> bool:
        """プロセスが動いているか。プロセスを持たない場合は制御サーバに問い合わせる。

        :return: 動いているか
        """
        if self.process is not None:
            return self.process.is_running()

        try:
            self.client.request("input", timeout=2.0)
        except ControlConnectionError:
            return False

        return True


def without_blanks(messages: list[str]) -> list[str]:
    """空白だけのメッセージを除く。

    :param messages: メッセージの一覧
    :return: 空白だけのものを除いた一覧
    """
    return [message for message in messages if message.strip()]


def remaining(deadline: float) -> float:
    """打ち切る時刻までの秒数を返す。過ぎていても、1 回の問い合わせができるだけの時間は返す。

    :param deadline: 打ち切る時刻 (time.monotonic() の値)
    :return: 秒数
    """
    return max(1.0, deadline - time.monotonic())
