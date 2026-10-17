import pytest

from hengband_mcp.keys import (
    MAX_KEYS_PER_CALL,
    Key,
    KeyNotationError,
    encode_byte,
    join_text,
    parse_keys,
)


def wires(text: str, encoding: str | None = None) -> list[str]:
    return [key.wire for key in parse_keys(text, encoding)]


def test_plain_characters_are_sent_one_by_one() -> None:
    assert parse_keys("q a") == [Key("q", "q"), Key(" ", " "), Key("a", "a")]


def test_named_keys_are_case_insensitive() -> None:
    assert wires("<ESC><esc><Enter><TAB><BS><SPACE>") == [
        "\\x1b",
        "\\x1b",
        "\\x0d",
        "\\x09",
        "\\x08",
        " ",
    ]


@pytest.mark.parametrize("name", ["C-x", "c-x", "CTRL-X", "^X", "^x"])
def test_control_keys(name: str) -> None:
    assert wires(f"<{name}>") == ["\\x18"]


def test_control_with_a_bracket_is_escape() -> None:
    assert wires("<C-[>") == ["\\x1b"]


def test_backslash_and_caret_are_escaped() -> None:
    # ゲームのマクロ表記では「\」と「^」が特別な意味を持つ
    assert wires("\\^") == ["\\\\", "\\^"]


def test_angle_brackets_without_a_name_are_literal() -> None:
    assert wires("<") == ["<"]
    assert wires(">") == [">"]
    assert wires("<>") == ["<", ">"]
    assert wires("< 6") == ["<", " ", "6"]
    assert wires("<R") == ["<", "R"]


def test_lt_and_gt_send_the_stairs_keys() -> None:
    assert wires("<LT><GT>") == ["<", ">"]


def test_named_key_text_is_kept_for_the_result() -> None:
    keys = parse_keys("R<ENTER>")
    assert [key.text for key in keys] == ["R", "<ENTER>"]
    assert join_text(keys) == "R<ENTER>"
    assert join_text(keys[1:]) == "<ENTER>"


@pytest.mark.parametrize("text", ["<ESq>", "<C-@>", "<F1>", "<CTRL-XY>", "<ENTRE>"])
def test_unknown_key_names_are_errors(text: str) -> None:
    with pytest.raises(KeyNotationError, match="unknown key name"):
        parse_keys(text)


@pytest.mark.parametrize("text", ["<ENTER", "q<ESC", "<C-x"])
def test_unclosed_key_names_are_errors(text: str) -> None:
    with pytest.raises(KeyNotationError, match="no closing"):
        parse_keys(text)


def test_non_ascii_needs_a_japanese_game() -> None:
    with pytest.raises(KeyNotationError, match="only ASCII"):
        parse_keys("あ")


@pytest.mark.parametrize(
    ("encoding", "expected"),
    [("cp932", ["\\x82\\xa0"]), ("euc_jp", ["\\xa4\\xa2"])],
)
def test_japanese_characters_are_sent_in_the_game_encoding(
    encoding: str, expected: list[str]
) -> None:
    assert wires("あ", encoding) == expected


def test_characters_without_a_representation_are_errors() -> None:
    with pytest.raises(KeyNotationError, match="no cp932 representation"):
        parse_keys("\U0001f600", "cp932")


def test_too_many_keys_are_rejected() -> None:
    parse_keys("6" * MAX_KEYS_PER_CALL)
    with pytest.raises(KeyNotationError, match="too many keys"):
        parse_keys("6" * (MAX_KEYS_PER_CALL + 1))


def test_encode_byte() -> None:
    assert encode_byte(ord("a")) == "a"
    assert encode_byte(0x5C) == "\\\\"
    assert encode_byte(0x5E) == "\\^"
    assert encode_byte(0x7F) == "\\x7f"
    assert encode_byte(0xFF) == "\\xff"
