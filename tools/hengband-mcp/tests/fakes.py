"""テストで制御サーバとゲームのプロセスの代わりをする。"""

from collections.abc import Callable
from typing import Any

from hengband_mcp.client import (
    ControlConnectionError,
    ControlRequestError,
    ControlTimeoutError,
)
from hengband_mcp.session import ESCAPE, SAVE_AND_QUIT

SCREEN_WIDTH = 80
SCREEN_HEIGHT = 24

KeyHandler = Callable[["FakeGame", str], None]


class FakeGame:
    """制御サーバの代わりに、受け取ったキーに応じて状態を変えながら応答する。

    キーの処理は on_key に渡した関数が行う。standard_keys() は ESC と ^X を本物のゲームと同じように扱う。
    """

    def __init__(
        self,
        on_key: KeyHandler | None = None,
        kind: str = "command",
        character_generated: bool = True,
    ) -> None:
        self.on_key = on_key or standard_keys
        self.kind = kind
        self.character_generated = character_generated
        self.message_sequence = 0
        self.disturb_sequence = 0
        self.closes = 0
        self.dead = False
        self.running = True
        self.saved = False
        self.quitting = False
        self.ignore_quit = False
        self.more_left = 0
        self.after_more = "command"
        self.death_screens = 3
        self.input_timeouts = 0
        self.exit_code: int | None = None
        self.messages: list[str] = []
        self.lines = [""] * SCREEN_HEIGHT
        self.snapshot: dict[str, Any] = {
            "player": {"level": 1, "hp": 10, "max_hp": 10, "x": 10, "y": 5},
            "floor": {"level": 0, "dungeon_name": "Town"},
            "visible_monsters": [],
        }
        self.keys: list[str] = []
        self.ops: list[str] = []

    def add_message(self, text: str) -> None:
        """メッセージ履歴に 1 行加える。"""
        self.messages.append(text)
        self.message_sequence += 1

    def show_more(self, count: int, after: str = "command") -> None:
        """-more- を count 回出し、閉じた後は after の入力待ちにする。"""
        self.kind = "more"
        self.more_left = count
        self.after_more = after

    def request(
        self, op: str, timeout: float | None = None, **params: Any
    ) -> dict[str, Any]:
        """ControlClient.request() と同じ形で応答する。"""
        if not self.running:
            raise ControlConnectionError("cannot talk to the game: connection refused")

        self.ops.append(op)
        if op == "input":
            if self.input_timeouts:
                self.input_timeouts -= 1
                raise ControlTimeoutError("the game did not respond within 1 seconds")

            return {
                "kind": self.kind,
                "message_sequence": self.message_sequence,
                "disturb_sequence": self.disturb_sequence,
                "character_generated": self.character_generated,
                "playing": self.character_generated and not self.dead,
                "dead": self.dead,
            }

        if op == "keys":
            key = params["keys"]
            self.keys.append(key)
            self.on_key(self, key)
            return {"pushed": len(key)}

        if op == "screen":
            return {
                "width": SCREEN_WIDTH,
                "height": SCREEN_HEIGHT,
                "lines": list(self.lines),
            }

        if op == "state":
            if not self.character_generated:
                raise ControlRequestError("state failed: the character is not ready")

            return self.snapshot

        if op == "messages":
            count = params["count"]
            return {"messages": self.messages[-count:] if count > 0 else []}

        if op == "quit":
            if not self.ignore_quit:
                self.running = False

            return {"quitting": True}

        if op == "info":
            return {
                "protocol_version": 3,
                "version": "Hengband 3.0.0 (test)",
                "japanese": False,
                "terms": [{"index": 0, "width": SCREEN_WIDTH, "height": SCREEN_HEIGHT}],
            }

        raise ControlRequestError(f"{op} failed: unknown op")

    def close(self) -> None:
        """ControlClient.close() と同じく接続を閉じる (回数を数えるだけ)。"""
        self.closes += 1


def standard_keys(game: FakeGame, key: str) -> None:
    """ESC と ^X (セーブして終了) を本物のゲームと同じように処理する。"""
    if key == ESCAPE:
        if game.kind == "more" and not game.dead:
            game.more_left -= 1
            if game.more_left <= 0:
                game.kind = game.after_more
        elif game.dead:
            # 死亡後の画面 (-more-、スクリーンダンプの確認、スコア) を 1 つずつ抜ける
            game.death_screens -= 1
            if game.death_screens <= 0:
                game.running = False
        elif game.quitting:
            # 「リターンキーか ESC キーを押して下さい」に ESC で答えると終了する
            game.running = False
        else:
            game.kind = "command"
    elif key == SAVE_AND_QUIT and game.kind == "command":
        game.saved = True
        game.quitting = True
        game.add_message("Saving game... done.")
        game.kind = "any_key"


class FakeProcess:
    """FakeGame を動かしているプロセスの代わり。"""

    pid = 12345

    def __init__(self, game: FakeGame) -> None:
        self.game = game
        self.killed = False
        self.closed = False

    @property
    def exit_code(self) -> int | None:
        return None if self.game.running else (self.game.exit_code or 0)

    def is_running(self) -> bool:
        return self.game.running

    def wait(self, timeout: float) -> bool:
        return not self.game.running

    def kill(self) -> None:
        if self.game.running:
            self.killed = True
            self.game.running = False

    def close(self) -> None:
        self.closed = True

    def log_tail(self, lines: int = 20) -> str:
        return "fake game log"
