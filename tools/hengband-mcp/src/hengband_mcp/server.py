"""MCP サーバの本体。LLM からのツール呼び出しを受けて、ゲームを起動・操作・終了する。

ツールの説明と instructions は LLM が読むため英語で書く (ゲームのメッセージは日本語版なら日本語のまま返す)。
"""

import argparse
import json
import logging
import sys
import threading
from collections.abc import Callable
from pathlib import Path
from typing import Annotated, Any, Literal, TypeVar

from mcp.server import MCPServer
from mcp.server.mcpserver.exceptions import ToolError
from mcp.types import TextContent, ToolAnnotations
from pydantic import Field

from . import __version__
from .client import ControlConnectionError, ControlError
from .keys import KeyNotationError
from .launcher import (
    GameProcess,
    LaunchError,
    LaunchOptions,
    default_instances_dir,
    find_game,
    launch,
    list_saves,
)
from .observation import View
from .session import GameSession, KeyRefusedError, StopError

logger = logging.getLogger(__name__)

T = TypeVar("T")

#: shutdown() がツールの処理の終わりを待つ秒数
SHUTDOWN_LOCK_TIMEOUT = 90.0

INSTRUCTIONS = """\
Hengband is a single-player roguelike (a Zangband/Angband variant): explore a 100-level dungeon and \
defeat the final boss. These tools drive a real game process through its keyboard: you send \
keystrokes and get back a JSON summary plus the screen text.

Workflow
1. start_game: a new character (the game opens on the character creation screens), or a save from list_saves.
2. Repeat: read the observation, then call send_keys with the next command. Every send_keys result \
already contains a fresh observation; call observe only to look again (e.g. inventory=true).
3. stop_game saves and closes the game. Death is permanent: a dead character cannot be continued.

Character creation (input "birth"; read each screen)
- Pick each choice by its letter and answer y when asked to confirm. <ESC> leaves the birth options \
screen. On the autoroller's minimum stats screen, press <ENTER> repeatedly to move down to Accept and \
start rolling. Then <ENTER> accepts the rolled stats, type a name and <ENTER>, <ENTER> keeps the \
background, and a last <ENTER> starts the game in town.

Observation (first content block, JSON)
- input: what the game waits for: command (ready for a command), more, yes_no, string, direction, \
target, item, store_command, any_key, birth (character creation), unknown (another screen or menu, \
e.g. the inventory list or the character sheet: read the screen; <ESC> leaves it). prompt: the \
question on the top line when the game is asking something.
- messages: game messages since your previous call, oldest first.
- player: level, hp/mp as [current, max], depth (0 = town; 1 level = 50 ft), pos {x, y}, status \
(blind, confused, ...), effects (status bar), food, light, speed.
- monsters: visible monsters, nearest first. dir is the direction with the digit that steps toward \
it, e.g. "NE(9)"; dist is the distance; dx/dy are offsets (east and south are positive).
The second content block is text: the map around you when input is "command", otherwise the whole \
screen (menus, prompts, character creation). Read choices and item letters from it.
Map (blank = not explored yet): @ you, . floor, # wall, % mineral vein, + closed door, ' open door, \
< up stairs, > down stairs, ^ trap (or mountain), ; rune, : rubble or plants, ~ water or lava (also \
light sources, chests and bones lying on the floor), digits = shop entrances in town (walk onto one to \
enter; 8 is your home), letters = monsters, other punctuation = items ($ gold, ! potion, ? scroll, \
= ring, " amulet, _ staff, - wand or rod, , food). Use l (look) to identify things.

Keys
- Plain characters are sent as typed. Special keys: <ESC>, <ENTER>, <TAB>, <BS>, <SPACE>, <C-x> \
(Ctrl+x). A '<' that does not start such a name is the up-stairs key; use <LT> / <GT> where a \
literal '<' / '>' would look like a key name.
- Keys are sent one at a time. -more- prompts are dismissed for you; their messages still appear in \
messages.
- With stop_on_disturb (default true), the rest of the keys is dropped when something interrupts you \
back at the command prompt (a monster comes into view, you take damage, a run or rest stops, you bump \
into a wall). outcome is then "disturbed" and remaining shows the keys not sent: look again before \
continuing.
- game_turns is the game time that passed while the keys were sent (one action at normal speed \
takes about 10; it varies). 0 means nothing happened in the game: the command was refused or did \
nothing (a rest that stopped at once because a monster is in view, *t with no monster in line of \
fire), or you only opened and closed a menu.
- <ESC> cancels any prompt or menu. Japanese builds accept Japanese text (names, inscriptions) as typed.

Commands (original keyset)
- Move with digits in numpad layout (7 8 9 / 4 6 / 1 2 3); walking into a monster attacks it. \
.<digit> runs, 5 or , stays in place, g picks up, R<ENTER> rests as needed, < and > take stairs, \
` travels to a location.
- Items: i inventory, e equipment, w wield/wear, t take off, d drop, k destroy, q quaff, r read, \
E eat, F refuel, f fire, v throw, a aim a wand, u use a staff, z zap a rod, A activate, I inspect.
- Magic and abilities: m cast (pick the book, then the spell), b browse, G study (some classes), \
U racial or class power.
- Terrain: o open, c close, T tunnel, D disarm, B bash, s search.
- Information: l look, * target, M dungeon map, L locate, C character sheet, <C-p> message history, \
<C-f> level feeling, ~ knowledge, ? help.
- Prompts: answer item prompts with the item letter shown on screen, yes/no prompts with y or n, and \
direction prompts with a digit, or with *t to aim at the nearest monster (* starts target selection, \
<SPACE> moves to the next monster, t picks it; t alone reuses the current target).

Rules
- Never press Q: it retires the character (permanent death). On the character creation screens Q quits.
- Use stop_game to save and quit, not <C-x>. <C-s> saves without quitting.
- Only one game runs at a time. On Windows only one Hengband can run on the PC.
"""

ViewParameter = Annotated[
    View,
    Field(
        description=(
            "Screen text to include: auto (the map at the command prompt, the whole screen otherwise), "
            "map, screen, or none."
        )
    ),
]

InventoryParameter = Annotated[
    bool, Field(description="Also include the inventory and equipment lists.")
]


def to_json(value: Any) -> str:
    """LLM に返す JSON を作る。日本語はエスケープせず、空白を省く。

    :param value: JSON にする値
    :return: JSON の文字列
    """
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def to_contents(summary: dict[str, Any], text: str | None) -> list[TextContent]:
    """観測をツールの結果にする。

    :param summary: JSON にする要約
    :param text: 画面のテキスト。含めない場合は None
    :return: 要約と画面のテキストの 2 つのブロック
    """
    contents = [TextContent(type="text", text=to_json(summary))]
    if text:
        contents.append(TextContent(type="text", text=text))

    return contents


class GameManager:
    """1 つのゲームを起動・操作・終了する。

    ツールは別々のスレッドから呼ばれることがあるため、ロックで 1 つずつ処理する。
    """

    def __init__(self, game: str | None, instance_dir: Path) -> None:
        """
        :param game: ゲームの実行ファイル。None なら find_game() で探す
        :param instance_dir: インスタンスのディレクトリ
        """
        self.game = game
        self.instance_dir = instance_dir
        self._lock = threading.Lock()
        self._session: GameSession | None = None
        self._process: GameProcess | None = None

    def start(
        self, options: LaunchOptions
    ) -> tuple[dict[str, Any], str | None]:
        """ゲームを起動して、最初の観測を返す。

        :param options: 起動方法
        :return: 観測の要約と画面のテキスト
        """
        with self._lock:
            if self._process is not None and self._process.is_running():
                raise ToolError(
                    "a game is already running; keep playing it, or call stop_game first"
                )

            self._forget()
            try:
                launched = launch(find_game(self.game), self.instance_dir, options)
            except LaunchError as error:
                raise ToolError(str(error)) from error

            self._process = launched.process
            self._session = GameSession(
                launched.client,
                launched.encoding,
                launched.process,
                options.allow_debug,
            )
            summary, text = self._guarded(lambda session: session.observe("auto"))
            game = {
                "version": launched.info.get("version"),
                "language": "ja" if launched.encoding else "en",
                "display": "window" if options.visible else "headless",
            }
            if options.save is not None:
                game["savefile"] = options.save

            return {"game": game, **summary}, text

    def run(self, action: Callable[[GameSession], T]) -> T:
        """動いているゲームを操作する。

        :param action: ゲームを操作する処理
        :return: 処理の結果
        """
        with self._lock:
            return self._guarded(action)

    def stop(self, save: bool) -> dict[str, Any]:
        """ゲームを終了させる。

        :param save: セーブしてから終了するか
        :return: 終了の結果
        """
        with self._lock:
            result = self._guarded(lambda session: session.stop(save))
            self._forget()
            if result.get("saved"):
                saves = list_saves(self.instance_dir)
                if saves:
                    result["savefile"] = saves[0]["name"]

            return {"stopped": True, **result}

    def saves(self) -> list[dict[str, Any]]:
        """インスタンスのセーブファイルを一覧にする。

        :return: セーブファイルの一覧
        """
        return list_saves(self.instance_dir)

    def shutdown(self) -> None:
        """サーバの終了時に、動いているゲームをセーブして終了させる。"""
        acquired = self._lock.acquire(timeout=SHUTDOWN_LOCK_TIMEOUT)
        try:
            session = self._session
            if session is None:
                return

            try:
                result = session.stop(save=True, force=True)
                logger.info("stopped the game: %s", result)
            except Exception:
                logger.exception("failed to stop the game")
            finally:
                if self._process is not None:
                    self._process.kill()

                self._forget()
        finally:
            if acquired:
                self._lock.release()

    def _guarded(self, action: Callable[[GameSession], T]) -> T:
        """ゲームを操作し、失敗を LLM に伝わるエラーにする。ロックを取ってから呼ぶこと。

        :param action: ゲームを操作する処理
        :return: 処理の結果
        """
        session = self._session
        if session is None:
            raise ToolError("no game is running; call start_game first")

        try:
            return action(session)
        except (KeyNotationError, KeyRefusedError, StopError) as error:
            raise ToolError(str(error)) from error
        except ControlConnectionError as error:
            process = self._process
            if process is not None and not process.is_running():
                exit_code = process.exit_code
                tail = process.log_tail(10)
                self._forget()
                message = f"the game is no longer running (exit code {exit_code}); call start_game to play again"
                if tail:
                    message += f"\nlast output of the game:\n{tail}"

                raise ToolError(message) from error

            raise ToolError(
                f"{error}; the game may still be busy, so call observe to check again"
            ) from error
        except ControlError as error:
            raise ToolError(str(error)) from error
        finally:
            # ツールを呼ぶ間はほかのクライアント (hbctl.py など) も制御サーバに接続できるようにする
            session.client.close()

    def _forget(self) -> None:
        """終了したゲームを手放す。"""
        if self._session is not None:
            self._session.client.close()

        if self._process is not None:
            self._process.close()

        self._session = None
        self._process = None


def create_server(manager: GameManager) -> MCPServer:
    """MCP サーバを作り、ツールを登録する。

    :param manager: ゲームを管理するオブジェクト
    :return: MCP サーバ
    """
    mcp = MCPServer(
        "hengband",
        title="Hengband",
        instructions=INSTRUCTIONS,
        version=__version__,
    )

    @mcp.tool(
        name="start_game",
        title="Start a game",
        description=(
            "Start Hengband and return the first observation. Without savefile a new character is "
            "created: the game opens on the character creation screens (input 'birth'), which you "
            "answer with send_keys while reading the screen text. With savefile, the saved game is "
            "loaded. Only one game runs at a time."
        ),
        annotations=ToolAnnotations(
            destructive_hint=False, idempotent_hint=False, open_world_hint=False
        ),
        structured_output=False,
    )
    def start_game(
        savefile: Annotated[
            str | None,
            Field(
                description="Name of a save to load (see list_saves). Omit to create a new character."
            ),
        ] = None,
        display: Annotated[
            Literal["headless", "window"],
            Field(
                description=(
                    "headless (default) runs without a window; window shows the normal game window "
                    "so a human can watch (Windows only)."
                )
            ),
        ] = "headless",
        seed: Annotated[
            int | None,
            Field(description="Fix the random seed for a reproducible game."),
        ] = None,
        allow_debug: Annotated[
            bool,
            Field(
                description=(
                    "Allow the debug commands (<C-a>) and wizard mode (<C-w>). Using them marks the "
                    "character as a cheater. For testing only."
                )
            ),
        ] = False,
    ) -> list[TextContent]:
        options = LaunchOptions(
            save=savefile,
            visible=display == "window",
            seed=seed,
            allow_debug=allow_debug,
        )
        return to_contents(*manager.start(options))

    @mcp.tool(
        name="send_keys",
        title="Send keys",
        description=(
            "Send keystrokes to the game and return the result followed by a fresh observation. "
            "Keys are sent one at a time, each processed before the next; -more- prompts are "
            "dismissed automatically. outcome is done, disturbed (stopped early, see stop_on_disturb), "
            "timeout (the game is still busy), or game_over (the character died; call stop_game). "
            "consumed lists the keys sent and remaining the keys not sent. game_turns is the game time "
            "that passed (about 10 per action at normal speed; 0 = nothing happened). Notation: plain "
            "characters as typed, plus <ESC> <ENTER> <TAB> <BS> <SPACE> <C-x>. Examples: '6' steps "
            "east, '.4' runs west, 'R<ENTER>' rests as needed, 'qa' quaffs the potion in slot a, "
            "'<ESC>' cancels."
        ),
        annotations=ToolAnnotations(
            destructive_hint=True, idempotent_hint=False, open_world_hint=False
        ),
        structured_output=False,
    )
    def send_keys(
        keys: Annotated[
            str,
            Field(description="Keys to send, at most 256 per call."),
        ],
        stop_on_disturb: Annotated[
            bool,
            Field(
                description=(
                    "Drop the remaining keys when something interrupts you back at the command prompt "
                    "(a monster appears, you take damage, a run or rest stops, you bump into a wall)."
                )
            ),
        ] = True,
        view: ViewParameter = "auto",
        inventory: InventoryParameter = False,
    ) -> list[TextContent]:
        def action(session: GameSession) -> tuple[dict[str, Any], str | None]:
            result = session.send_keys(keys, stop_on_disturb)
            head: dict[str, Any] = {
                "outcome": result.outcome,
                "consumed": result.consumed,
            }
            if result.remaining:
                head["remaining"] = result.remaining

            if result.more_dismissed:
                head["more_dismissed"] = result.more_dismissed

            if result.game_turns is not None:
                head["game_turns"] = result.game_turns

            if result.outcome == "timeout":
                head["note"] = "the game is still processing; call observe to see the result"
                return head, None

            summary, text = session.observe(view, inventory, refresh=False)
            return {**head, **summary}, text

        return to_contents(*manager.run(action))

    @mcp.tool(
        name="observe",
        title="Observe",
        description=(
            "Return the current observation without sending keys: what the game waits for, messages "
            "since your previous call, your status, visible monsters, and the map or screen text."
        ),
        annotations=ToolAnnotations(read_only_hint=True, open_world_hint=False),
        structured_output=False,
    )
    def observe(
        view: ViewParameter = "auto", inventory: InventoryParameter = False
    ) -> list[TextContent]:
        return to_contents(
            *manager.run(lambda session: session.observe(view, inventory))
        )

    @mcp.tool(
        name="get_screen",
        title="Get the screen",
        description="Return the whole game screen as plain text, e.g. to read a menu in full.",
        annotations=ToolAnnotations(read_only_hint=True, open_world_hint=False),
        structured_output=False,
    )
    def get_screen() -> str:
        return manager.run(lambda session: session.screen_text())

    @mcp.tool(
        name="stop_game",
        title="Stop the game",
        description=(
            "End the game. With save=true (default) the game is saved first and can be continued "
            "later with start_game(savefile=...); the result names the savefile. With save=false the "
            "game is closed without saving, losing the progress since the last save."
        ),
        annotations=ToolAnnotations(
            destructive_hint=True, idempotent_hint=False, open_world_hint=False
        ),
        structured_output=False,
    )
    def stop_game(
        save: Annotated[
            bool, Field(description="Save before quitting (default true).")
        ] = True,
    ) -> str:
        return to_json(manager.stop(save))

    @mcp.tool(
        name="list_saves",
        title="List saves",
        description="List the saved games (newest first). Pass a name to start_game to continue it.",
        annotations=ToolAnnotations(read_only_hint=True, open_world_hint=False),
        structured_output=False,
    )
    def list_saves() -> str:
        return to_json({"saves": manager.saves()})

    return mcp


def parse_arguments(argv: list[str] | None) -> argparse.Namespace:
    """コマンドライン引数を解釈する。

    :param argv: 引数。None ならプロセスの引数
    :return: 解釈した結果
    """
    parser = argparse.ArgumentParser(
        prog="hengband-mcp",
        description="MCP server that lets LLM agents play Hengband (stdio transport).",
    )
    parser.add_argument(
        "--game",
        help=(
            "the game executable (Hengband.exe on Windows, src/hengband elsewhere); defaults to "
            "$HENGBAND_GAME or the build in the repository that contains this server"
        ),
    )
    parser.add_argument(
        "--instances-dir",
        type=Path,
        help="where game instances live (default: %%LOCALAPPDATA%%\\hengband-mcp\\instances on Windows)",
    )
    parser.add_argument(
        "--instance",
        default="default",
        help="name of the instance directory, which holds its own saves and settings (default: default)",
    )
    parser.add_argument(
        "--log-level",
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR"],
        help="log level of the messages written to stderr",
    )
    arguments = parser.parse_args(argv)
    if Path(arguments.instance).name != arguments.instance or arguments.instance in ("", ".", ".."):
        parser.error(f"invalid instance name: {arguments.instance!r}")

    return arguments


def main(argv: list[str] | None = None) -> None:
    """MCP サーバを stdio で動かす。

    :param argv: コマンドライン引数。None ならプロセスの引数
    """
    arguments = parse_arguments(argv)
    # 標準出力は MCP の通信に使うため、ログは標準エラー出力へ出す
    logging.basicConfig(
        stream=sys.stderr,
        level=arguments.log_level,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )
    instances_dir = arguments.instances_dir or default_instances_dir()
    manager = GameManager(arguments.game, instances_dir / arguments.instance)
    server = create_server(manager)
    try:
        server.run("stdio")
    finally:
        manager.shutdown()


if __name__ == "__main__":
    main()
