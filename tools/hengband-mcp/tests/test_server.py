import asyncio
import json
from collections.abc import Awaitable, Callable
from pathlib import Path
from typing import Any

import pytest
from mcp import Client

from fakes import FakeGame, FakeProcess, standard_keys
from hengband_mcp import server
from hengband_mcp.launcher import LaunchedGame, LaunchOptions
from hengband_mcp.server import GameManager, create_server
from hengband_mcp.session import SAVE_AND_QUIT


class Harness:
    """ゲームを起動する代わりに FakeGame を返すようにして、MCP サーバを動かす。"""

    def __init__(self, monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
        self.instance = tmp_path / "instance"
        self.games: list[FakeGame] = []
        self.options: list[LaunchOptions] = []
        monkeypatch.setattr(server, "find_game", lambda explicit: tmp_path / "Hengband.exe")
        monkeypatch.setattr(server, "launch", self.launch)
        self.manager = GameManager(None, self.instance)
        self.server = create_server(self.manager)

    @property
    def game(self) -> FakeGame:
        return self.games[-1]

    def launch(self, game: Path, instance_dir: Path, options: LaunchOptions) -> LaunchedGame:
        fake = FakeGame(self.on_key)
        fake.lines[0] = "Welcome to Hengband."
        fake.lines[5] = " " * 13 + "#@."
        fake.add_message("Welcome to Hengband.")
        self.games.append(fake)
        self.options.append(options)
        info = {"version": "Hengband 3.0.0 (test)", "japanese": False}
        return LaunchedGame(FakeProcess(fake), fake, None, info)  # type: ignore[arg-type]

    def on_key(self, game: FakeGame, key: str) -> None:
        standard_keys(game, key)
        if key == "6":
            game.add_message("You walk.")
        elif key == "R":
            game.input_timeouts = 1
        elif key == SAVE_AND_QUIT:
            save = self.instance / "lib" / "save" / "Alice"
            save.parent.mkdir(parents=True, exist_ok=True)
            save.write_bytes(b"save")

    def run(self, scenario: Callable[[Client], Awaitable[None]]) -> None:
        async def main() -> None:
            async with Client(self.server) as client:
                await scenario(client)

        asyncio.run(main())


@pytest.fixture
def harness(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> Harness:
    return Harness(monkeypatch, tmp_path)


def texts(result: Any) -> list[str]:
    return [content.text for content in result.content]


def test_a_game_from_start_to_stop(harness: Harness) -> None:
    async def scenario(client: Client) -> None:
        result = await client.call_tool("start_game", {"seed": 7})
        assert not result.is_error
        summary = json.loads(texts(result)[0])
        assert summary["game"] == {
            "version": "Hengband 3.0.0 (test)",
            "language": "en",
            "display": "headless",
        }
        assert summary["input"] == "command"
        assert summary["messages"] == ["Welcome to Hengband."]
        assert texts(result)[1] == "#@."

        result = await client.call_tool("send_keys", {"keys": "66"})
        summary = json.loads(texts(result)[0])
        assert summary["outcome"] == "done"
        assert summary["consumed"] == "66"
        assert "remaining" not in summary
        assert summary["messages"] == ["You walk.", "You walk."]
        assert summary["view"] == "map"
        # ツールを呼び終えたら、ほかのクライアントが使えるよう制御サーバとの接続を閉じる
        assert harness.game.closes > 0

        result = await client.call_tool("observe", {"view": "none", "inventory": True})
        summary = json.loads(texts(result)[0])
        assert summary["messages"] == []
        assert summary["inventory"] == []
        assert len(result.content) == 1

        result = await client.call_tool("get_screen", {})
        assert texts(result)[0].startswith("Welcome to Hengband.")

        result = await client.call_tool("stop_game", {})
        assert json.loads(texts(result)[0]) == {
            "stopped": True,
            "saved": True,
            "savefile": "Alice",
        }

        result = await client.call_tool("list_saves", {})
        saves = json.loads(texts(result)[0])["saves"]
        assert [save["name"] for save in saves] == ["Alice"]

        result = await client.call_tool("observe", {})
        assert result.is_error
        assert "call start_game" in texts(result)[0]

    harness.run(scenario)
    assert harness.options == [LaunchOptions(seed=7)]
    assert harness.game.saved
    assert not harness.game.running


def test_start_game_passes_the_options(harness: Harness) -> None:
    async def scenario(client: Client) -> None:
        arguments = {"savefile": "Alice", "display": "window", "allow_debug": True}
        result = await client.call_tool("start_game", arguments)
        summary = json.loads(texts(result)[0])
        assert summary["game"]["savefile"] == "Alice"
        assert summary["game"]["display"] == "window"

        result = await client.call_tool("send_keys", {"keys": "<C-a>"})
        assert not result.is_error

    harness.run(scenario)
    assert harness.options == [LaunchOptions(save="Alice", visible=True, allow_debug=True)]


def test_only_one_game_runs_at_a_time(harness: Harness) -> None:
    async def scenario(client: Client) -> None:
        await client.call_tool("start_game", {})
        result = await client.call_tool("start_game", {})
        assert result.is_error
        assert "already running" in texts(result)[0]

    harness.run(scenario)
    assert len(harness.games) == 1


def test_send_keys_reports_bad_keys(harness: Harness) -> None:
    async def scenario(client: Client) -> None:
        await client.call_tool("start_game", {})

        result = await client.call_tool("send_keys", {"keys": "<C-x>"})
        assert result.is_error
        assert "stop_game" in texts(result)[0]

        result = await client.call_tool("send_keys", {"keys": "<C-a>"})
        assert result.is_error
        assert "allow_debug" in texts(result)[0]

        result = await client.call_tool("send_keys", {"keys": "<ESq>"})
        assert result.is_error
        assert "unknown key name" in texts(result)[0]

    harness.run(scenario)
    assert harness.game.keys == []


def test_send_keys_reports_a_busy_game(harness: Harness) -> None:
    async def scenario(client: Client) -> None:
        await client.call_tool("start_game", {})
        result = await client.call_tool("send_keys", {"keys": "R6"})
        summary = json.loads(texts(result)[0])
        assert summary == {
            "outcome": "timeout",
            "consumed": "R",
            "remaining": "6",
            "note": "the game is still processing; call observe to see the result",
        }
        assert len(result.content) == 1

        result = await client.call_tool("observe", {})
        assert json.loads(texts(result)[0])["input"] == "command"

    harness.run(scenario)


def test_a_game_that_exited_is_reported_and_can_be_restarted(harness: Harness) -> None:
    async def scenario(client: Client) -> None:
        await client.call_tool("start_game", {})
        harness.game.running = False
        harness.game.exit_code = 3

        result = await client.call_tool("send_keys", {"keys": "6"})
        assert result.is_error
        message = texts(result)[0]
        assert "no longer running (exit code 3)" in message
        assert "fake game log" in message

        result = await client.call_tool("start_game", {})
        assert not result.is_error

    harness.run(scenario)
    assert len(harness.games) == 2


def test_shutdown_saves_the_running_game(harness: Harness) -> None:
    async def scenario(client: Client) -> None:
        await client.call_tool("start_game", {})

    harness.run(scenario)
    harness.manager.shutdown()

    assert harness.game.saved
    assert not harness.game.running


def test_main_rejects_an_instance_path() -> None:
    with pytest.raises(SystemExit):
        server.parse_arguments(["--instance", "../other"])
