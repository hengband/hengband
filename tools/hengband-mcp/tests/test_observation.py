import pytest

from hengband_mcp.observation import (
    MAX_VISIBLE_MONSTERS,
    InputState,
    build_observation,
    direction_label,
    distance,
    extract_map,
    screen_lines,
    slice_cells,
    summarize_items,
    summarize_monsters,
    summarize_player,
    to_cells,
)

COMMAND = InputState("command", 0, 0, True, True, False)


@pytest.mark.parametrize(
    ("dx", "dy", "expected"),
    [
        (0, 0, "here"),
        (3, 0, "E(6)"),
        (-3, 0, "W(4)"),
        (0, -2, "N(8)"),
        (0, 2, "S(2)"),
        (2, 2, "SE(3)"),
        (-4, -3, "NW(7)"),
        (5, -2, "NE(9)"),
        (5, 1, "E(6)"),
        (-1, 5, "S(2)"),
    ],
)
def test_direction_label(dx: int, dy: int, expected: str) -> None:
    assert direction_label(dx, dy) == expected


def test_distance_matches_the_game() -> None:
    assert distance(3, 1) == 3
    assert distance(1, -3) == 3
    assert distance(4, 4) == 6
    assert distance(0, 0) == 0


def test_full_width_characters_take_two_cells() -> None:
    assert to_cells("あa", "cp932") == ["あ", None, "a"]
    assert to_cells("あa", None) == ["あ", "a"]


def test_slicing_through_a_full_width_character_leaves_a_space() -> None:
    cells = to_cells("aあいb", "cp932")
    assert slice_cells(cells, 0, 2, "cp932") == "a "
    assert slice_cells(cells, 2, 6, "cp932") == " いb"


def test_extract_map_drops_the_side_panel_and_status_line() -> None:
    lines = [""] * 24
    lines[0] = "You feel something in the air."
    lines[2] = "Level      1".ljust(13) + " ......"
    lines[3] = "HP    10/  10".ljust(13) + "  .@.#"
    lines[4] = "AC       12".ljust(13) + " ....."
    lines[23] = "Town  Full"

    assert extract_map(lines, None) == "......\n .@.#\n....."


def test_extract_map_with_japanese_side_panel() -> None:
    lines = [""] * 24
    # 左の能力値欄の全角文字は 2 桁を占める (「レベル」で 6 桁)
    lines[1] = "レベル" + " " * 6 + "1" + "#.@"

    assert extract_map(lines, "cp932") == "#.@"


def test_screen_lines_draws_the_walls_of_the_windows_font_as_hashes() -> None:
    # ウィンドウを表示した Windows 版は、壁や鉱脈を 0x7F (塗りつぶした四角) で描く
    screen = {"lines": ["\x7f\x7f+\x7f", "\x7f.@\x7f"]}

    assert screen_lines(screen) == ["##+#", "#.@#"]
    assert screen_lines(None) == []


def test_summarize_player() -> None:
    snapshot = {
        "player": {
            "level": 12,
            "hp": 30,
            "max_hp": 80,
            "mp": 5,
            "max_mp": 20,
            "exp": 1500,
            "exp_to_advance": 1700,
            "gold": 340,
            "ac": 25,
            "speed_display": {"value": 10},
            "food_state": "normal",
            "light_radius": 2,
            "x": 40,
            "y": 12,
            "status": {
                "blind": False,
                "poisoned": True,
                "stunned": True,
                "stun_rank_name": "heavy",
                "cut": False,
            },
            "status_bar": [{"key": "hero"}, {"key": "bless"}],
            "action": {"kind": "rest", "rest": -2},
            "recalling": True,
        },
        "floor": {"level": 5, "dungeon_name": "Angband"},
        "clock": {"day": 3, "hour": 7, "minute": 5},
    }

    assert summarize_player(snapshot) == {
        "level": 12,
        "hp": [30, 80],
        "mp": [5, 20],
        "exp": 1500,
        "exp_to_advance": 1700,
        "gold": 340,
        "ac": 25,
        "speed": 10,
        "food": "normal",
        "light": 2,
        "place": "Angband",
        "depth": 5,
        "pos": {"x": 40, "y": 12},
        "status": ["poisoned", "stun:heavy"],
        "effects": ["hero", "bless"],
        "action": "rest:-2",
        "recalling": True,
        "clock": "day 3 07:05",
    }


def test_summarize_monsters_sorts_by_distance() -> None:
    monsters = [
        {"name": "Grip, Farmer Maggot's Dog", "x": 15, "y": 5, "level": 2, "health": "unhurt"},
        {"name": "jackal", "x": 11, "y": 6, "health": "wounded", "asleep": True},
        {"name": "Fang", "x": 7, "y": 5, "hallucinated": True},
    ]

    entries, omitted = summarize_monsters(monsters, 10, 5, MAX_VISIBLE_MONSTERS)

    assert omitted == 0
    assert entries == [
        {
            "name": "jackal",
            "dir": "SE(3)",
            "dist": 1,
            "dx": 1,
            "dy": 1,
            "health": "wounded",
            "flags": ["asleep"],
        },
        {"name": "(hallucination)", "dir": "W(4)", "dist": 3, "dx": -3, "dy": 0},
        {
            "name": "Grip, Farmer Maggot's Dog",
            "dir": "E(6)",
            "dist": 5,
            "dx": 5,
            "dy": 0,
            "level": 2,
        },
    ]


def test_summarize_monsters_limits_the_count() -> None:
    monsters = [{"name": f"m{index}", "x": index, "y": 0} for index in range(5)]

    entries, omitted = summarize_monsters(monsters, 0, 0, 3)

    assert [entry["name"] for entry in entries] == ["m0", "m1", "m2"]
    assert omitted == 2


def test_summarize_items() -> None:
    snapshot = {
        "inventory": [
            {"slot": "a", "name": "Rations of Food", "count": 4},
            {"slot": "b", "name": "a Scroll of Phase Door", "count": 1},
        ],
        "equipment": [{"slot": "weapon", "name": "a Dagger (1d4) (+0,+0)"}],
    }

    assert summarize_items(snapshot) == {
        "inventory": ["a) Rations of Food x4", "b) a Scroll of Phase Door"],
        "equipment": ["weapon: a Dagger (1d4) (+0,+0)"],
    }


def test_build_observation_at_a_prompt() -> None:
    state = InputState("yes_no", 7, 0, True, True, False)
    screen = {"lines": ["Really quit? [y/n] ", "", "Town"]}

    summary, text = build_observation(state, screen, None, ["Hello."], 2, "auto", None)

    assert summary == {
        "input": "yes_no",
        "prompt": "Really quit? [y/n]",
        "messages": ["Hello."],
        "messages_omitted": 2,
        "view": "screen",
    }
    assert text == "Really quit? [y/n]\n\nTown"


def test_build_observation_at_the_command_prompt() -> None:
    snapshot = {
        "player": {"hp": 5, "max_hp": 10, "x": 3, "y": 3},
        "floor": {"level": 1},
        "visible_monsters": [{"name": "kobold", "x": 4, "y": 3}],
        "detected_monsters": [{"name": "orc", "x": 3, "y": 20}],
        "inventory": [],
        "equipment": [],
    }
    lines = [""] * 24
    lines[0] = "The kobold misses you."
    lines[3] = " " * 13 + "@k"

    summary, text = build_observation(COMMAND, {"lines": lines}, snapshot, [], 0, "auto", None, True)

    assert "prompt" not in summary
    assert summary["player"]["hp"] == [5, 10]
    assert summary["monsters"][0]["dir"] == "E(6)"
    assert summary["detected_monsters"][0]["dir"] == "S(2)"
    assert summary["inventory"] == []
    assert summary["view"] == "map"
    assert text == "@k"


def test_build_observation_marks_a_dead_character() -> None:
    state = InputState("more", 0, 0, True, False, True)
    summary, text = build_observation(state, None, None, [], 0, "none", None)

    assert summary["dead"] is True
    assert text is None
