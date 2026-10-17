import pytest

from fakes import FakeGame, FakeProcess, standard_keys
from hengband_mcp.keys import encode_byte
from hengband_mcp.session import (
    ESCAPE,
    MAX_ESCAPES,
    MAX_MESSAGES,
    SAVE_AND_QUIT,
    GameSession,
    KeyRefusedError,
    KeysResult,
    StopError,
)

ENTER = encode_byte(0x0D)


def make_session(game: FakeGame, allow_debug: bool = False) -> GameSession:
    return GameSession(game, None, FakeProcess(game), allow_debug)  # type: ignore[arg-type]


def test_send_keys_waits_for_each_key_to_be_processed() -> None:
    game = FakeGame()
    result = make_session(game).send_keys("66")

    assert result == KeysResult("done", "66", "", 0)
    assert game.keys == ["6", "6"]
    assert game.ops == ["input", "keys", "input", "keys", "input"]


def test_send_keys_sends_named_keys_as_one_key() -> None:
    game = FakeGame()
    result = make_session(game).send_keys("R<ENTER>")

    assert result.consumed == "R<ENTER>"
    assert game.keys == ["R", ENTER]


def test_send_keys_dismisses_more_prompts() -> None:
    def on_key(game: FakeGame, key: str) -> None:
        standard_keys(game, key)
        if key == "6":
            game.add_message("You hit the jackal.")
            game.add_message("You have slain the jackal.")
            game.show_more(2)

    game = FakeGame(on_key)
    result = make_session(game).send_keys("66")

    assert result == KeysResult("done", "66", "", 4)
    assert game.keys == ["6", ESCAPE, ESCAPE, "6", ESCAPE, ESCAPE]


def test_send_keys_dismisses_a_pending_more_before_the_first_key() -> None:
    game = FakeGame()
    game.show_more(1)
    result = make_session(game).send_keys("6")

    assert result.more_dismissed == 1
    assert game.keys == [ESCAPE, "6"]


def test_send_keys_stops_when_disturbed_at_the_command_prompt() -> None:
    def on_key(game: FakeGame, key: str) -> None:
        if key == "6":
            game.disturb_sequence += 1

    game = FakeGame(on_key)
    result = make_session(game).send_keys("666")

    assert result == KeysResult("disturbed", "6", "66", 0)
    assert game.keys == ["6"]


def test_send_keys_reports_done_when_the_last_key_is_disturbed() -> None:
    def on_key(game: FakeGame, key: str) -> None:
        if key == "6":
            game.disturb_sequence += 1

    result = make_session(FakeGame(on_key)).send_keys("6")

    assert result.outcome == "done"


def test_send_keys_keeps_going_without_stop_on_disturb() -> None:
    def on_key(game: FakeGame, key: str) -> None:
        game.disturb_sequence += 1

    game = FakeGame(on_key)
    result = make_session(game).send_keys("666", stop_on_disturb=False)

    assert result == KeysResult("done", "666", "", 0)


def test_send_keys_does_not_stop_on_a_disturbance_inside_a_prompt() -> None:
    def on_key(game: FakeGame, key: str) -> None:
        if key == "R":
            game.kind = "string"
            game.disturb_sequence += 1
        elif key == ENTER:
            game.kind = "command"

    game = FakeGame(on_key)
    result = make_session(game).send_keys("R<ENTER>6")

    assert result.outcome == "done"
    assert game.keys == ["R", ENTER, "6"]


def test_send_keys_stops_when_the_character_dies() -> None:
    def on_key(game: FakeGame, key: str) -> None:
        if key == "6":
            game.dead = True
            game.add_message("You die.")
            game.show_more(1)

    game = FakeGame(on_key)
    result = make_session(game).send_keys("66")

    # 死亡時の -more- は最後のメッセージを残すため閉じない
    assert result == KeysResult("game_over", "6", "6", 0)
    assert game.keys == ["6"]


def test_send_keys_reports_a_timeout_while_the_game_is_busy() -> None:
    def on_key(game: FakeGame, key: str) -> None:
        if key == "R":
            game.input_timeouts = 1

    game = FakeGame(on_key)
    session = make_session(game)
    result = session.send_keys("R6")

    assert result == KeysResult("timeout", "R", "6", 0)
    assert session.state is None


def test_send_keys_reports_a_timeout_after_the_deadline() -> None:
    game = FakeGame()
    result = make_session(game).send_keys("66", timeout=-1)

    assert result == KeysResult("timeout", "", "66", 0)
    assert game.keys == []


def test_send_keys_refuses_save_and_quit() -> None:
    game = FakeGame()
    with pytest.raises(KeyRefusedError, match="stop_game"):
        make_session(game).send_keys("i<C-x>")

    assert game.keys == []


def test_send_keys_refuses_debug_commands_unless_allowed() -> None:
    game = FakeGame()
    with pytest.raises(KeyRefusedError, match="allow_debug"):
        make_session(game).send_keys("<C-a>")

    with pytest.raises(KeyRefusedError, match="allow_debug"):
        make_session(game).send_keys("<C-w>")

    assert game.keys == []
    make_session(game, allow_debug=True).send_keys("<C-a>")
    assert game.keys == [encode_byte(0x01)]


def test_new_messages_starts_with_the_recent_history() -> None:
    game = FakeGame()
    for index in range(15):
        game.add_message(f"m{index}")

    messages, omitted = make_session(game).new_messages(game.message_sequence)

    assert messages == [f"m{index}" for index in range(5, 15)]
    assert omitted == 0


def test_new_messages_returns_only_new_lines() -> None:
    game = FakeGame()
    game.add_message("Welcome to Hengband.")
    session = make_session(game)
    session.new_messages(game.message_sequence)

    assert session.new_messages(game.message_sequence) == ([], 0)
    game.add_message("You have no more Flasks of oil.")
    game.add_message("You feel hungry.")
    assert session.new_messages(game.message_sequence) == (
        ["You have no more Flasks of oil.", "You feel hungry."],
        0,
    )


def test_new_messages_skips_blank_lines() -> None:
    game = FakeGame()
    session = make_session(game)
    session.new_messages(game.message_sequence)

    # 新規ゲームの開始時に、履歴の区切りとして空白だけの行が加わる
    for message in (" ", "  ", "====================", " ", "  ", "You feel hungry."):
        game.add_message(message)

    assert session.new_messages(game.message_sequence) == (
        ["====================", "You feel hungry."],
        0,
    )


def test_new_messages_notices_a_repeated_message() -> None:
    game = FakeGame()
    game.add_message("You miss the jackal.")
    session = make_session(game)
    session.new_messages(game.message_sequence)

    # 同じメッセージが続くと、履歴の行は増えずに最新行の回数だけが変わる
    game.messages[-1] = "You miss the jackal. <x2>"
    assert session.new_messages(game.message_sequence) == (
        ["You miss the jackal. <x2>"],
        0,
    )

    game.messages[-1] = "You miss the jackal. <x3>"
    game.add_message("The jackal bites you.")
    assert session.new_messages(game.message_sequence) == (
        ["You miss the jackal. <x3>", "The jackal bites you."],
        0,
    )


def test_new_messages_omits_lines_over_the_limit() -> None:
    game = FakeGame()
    session = make_session(game)
    session.new_messages(game.message_sequence)
    for index in range(MAX_MESSAGES + 10):
        game.add_message(f"m{index}")

    messages, omitted = session.new_messages(game.message_sequence)

    assert messages == [f"m{index}" for index in range(10, MAX_MESSAGES + 10)]
    assert omitted == 10


def test_observe_shows_the_map_at_the_command_prompt() -> None:
    game = FakeGame()
    game.lines[0] = "You see a Scroll of Light."
    game.lines[5] = "Human        #####"
    game.lines[6] = "Warrior      #.@.#"
    game.lines[7] = "             #####"
    game.lines[23] = "Town"
    game.add_message("You see a Scroll of Light.")

    summary, text = make_session(game).observe()

    assert summary["input"] == "command"
    assert "prompt" not in summary
    assert summary["messages"] == ["You see a Scroll of Light."]
    assert summary["player"]["hp"] == [10, 10]
    assert summary["view"] == "map"
    assert text == "#####\n#.@.#\n#####"


def test_observe_shows_the_whole_screen_and_the_prompt_in_a_prompt() -> None:
    game = FakeGame(kind="item")
    game.lines[0] = "Quaff which potion? "
    game.lines[1] = "  a) a Potion of Cure Light Wounds"

    summary, text = make_session(game).observe()

    assert summary["prompt"] == "Quaff which potion?"
    assert summary["view"] == "screen"
    assert text is not None and text.startswith(
        "Quaff which potion?\n  a) a Potion of Cure Light Wounds"
    )


def test_observe_skips_the_state_before_the_character_exists() -> None:
    game = FakeGame(kind="birth", character_generated=False)
    game.lines[0] = "Choose a race"

    summary, _ = make_session(game).observe()

    assert "player" not in summary
    assert "state" not in game.ops


def test_observe_without_a_view_skips_the_screen_at_the_command_prompt() -> None:
    game = FakeGame()
    summary, text = make_session(game).observe(view="none")

    assert text is None
    assert "view" not in summary
    assert "screen" not in game.ops


def test_stop_saves_after_leaving_a_prompt() -> None:
    game = FakeGame(kind="item")
    process = FakeProcess(game)
    result = GameSession(game, None, process).stop(save=True)  # type: ignore[arg-type]

    assert result == {"saved": True}
    assert game.keys == [ESCAPE, SAVE_AND_QUIT, ESCAPE]
    assert game.saved
    assert not game.running
    assert not process.killed


def test_stop_keeps_the_game_running_when_it_cannot_save() -> None:
    game = FakeGame(lambda game, key: None, kind="string")

    with pytest.raises(StopError, match="save=false"):
        make_session(game).stop(save=True)

    assert game.keys == [ESCAPE] * MAX_ESCAPES
    assert game.running


def test_stop_with_force_quits_without_saving_when_it_cannot_save() -> None:
    game = FakeGame(lambda game, key: None, kind="string")
    result = make_session(game).stop(save=True, force=True)

    assert result == {"saved": False}
    assert game.ops[-1] == "quit"
    assert not game.running


def test_stop_without_saving_quits_the_game() -> None:
    game = FakeGame()
    result = make_session(game).stop(save=False)

    assert result == {"saved": False}
    assert game.keys == []
    assert game.ops[-1] == "quit"
    assert not game.saved


def test_stop_leaves_the_screens_after_death() -> None:
    game = FakeGame(kind="more")
    game.dead = True
    result = make_session(game).stop(save=True)

    assert result == {"saved": False, "dead": True}
    assert game.keys == [ESCAPE] * 3
    assert "quit" not in game.ops


def test_stop_before_the_character_exists_saves_nothing() -> None:
    game = FakeGame(kind="birth", character_generated=False)
    result = make_session(game).stop(save=True)

    assert result["saved"] is False
    assert "not created" in result["note"]
    assert game.keys == []


def test_stop_kills_a_game_that_does_not_quit() -> None:
    game = FakeGame()
    game.ignore_quit = True
    process = FakeProcess(game)
    result = GameSession(game, None, process).stop(save=False)  # type: ignore[arg-type]

    assert result == {"saved": False, "killed": True}
    assert process.killed
