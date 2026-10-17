"""LLM が書くキー表記を、制御サーバの keys op に渡すマクロ表記へ変換する。

LLM には「<ESC>」「<C-x>」のような名前付きの表記を使わせる。ゲームのマクロ表記
（「\\e」「^X」）は JSON の中でバックスラッシュが二重になり書き間違えやすいため使わせない。

変換後のキー列は「\\\\」「\\^」「\\xNN」だけをエスケープとして使う。ゲーム側の
text_to_ascii() は未知のエスケープ（「\\q」など）を黙って捨てるため、それ以外は生成しない。
"""

import re
from dataclasses import dataclass

#: 名前で指定できるキーと、その文字コード
NAMED_KEYS: dict[str, int] = {
    "ESC": 0x1B,
    "ESCAPE": 0x1B,
    "ENTER": 0x0D,
    "RETURN": 0x0D,
    "TAB": 0x09,
    "BS": 0x08,
    "BACKSPACE": 0x08,
    "SPACE": 0x20,
    "LT": 0x3C,
    "GT": 0x3E,
}

#: 1 回の send_keys で送れるキーの数
MAX_KEYS_PER_CALL = 256

#: Ctrl と組み合わせられる文字（「<C-x>」の x）。「^@」は NUL になり送れないため除く
CONTROL_CHARACTERS = frozenset("ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_")

#: 「<」と「>」の間がこの形なら、キーの名前を書いたものとみなす
#: （名前として解釈できなければ、書き間違いとしてエラーにする）
NAME_PATTERN = re.compile(r"[A-Za-z0-9@\[\]\\^_-]+")

#: 「>」で閉じていない名前がこの形で始まるなら、「>」を書き忘れたものとみなす
#: （「<ENTER」「<C-x」など。「<R」のような 1 文字は上り階段とコマンドとして扱う）
UNCLOSED_NAME_PATTERN = re.compile(r"[A-Za-z][A-Za-z-]")


class KeyNotationError(ValueError):
    """キー表記を解釈できない。"""


@dataclass(frozen=True)
class Key:
    """ゲームへ 1 回で送るキー。

    :param text: LLM が書いた元の表記（応答で消費したキーを示すのに使う）
    :param wire: 制御サーバのマクロ表記に変換したキー列
    """

    text: str
    wire: str


def encode_byte(value: int) -> str:
    """1 バイトをマクロ表記にする。

    :param value: バイトの値
    :return: マクロ表記の文字列
    """
    if value == 0x5C:
        return "\\\\"

    if value == 0x5E:
        return "\\^"

    if 0x20 <= value < 0x7F:
        return chr(value)

    return f"\\x{value:02x}"


def parse_named_key(name: str) -> int | None:
    """「<...>」の中身を文字コードにする。

    :param name: 「<」と「>」の間の文字列
    :return: 文字コード。名前として解釈できない場合は None
    """
    upper = name.upper()
    if upper in NAMED_KEYS:
        return NAMED_KEYS[upper]

    for prefix in ("C-", "CTRL-", "^"):
        if upper.startswith(prefix) and len(upper) == len(prefix) + 1:
            target = upper[-1]
            if target in CONTROL_CHARACTERS:
                return ord(target) & 0x1F

    return None


def encode_character(character: str, encoding: str | None) -> str:
    """1 文字をマクロ表記にする。

    :param character: 変換する文字
    :param encoding: 全角文字を送るときのゲームの文字コード。None なら ASCII だけを受け付ける
    :return: マクロ表記の文字列
    """
    code = ord(character)
    if code < 0x80:
        return encode_byte(code)

    if encoding is None:
        raise KeyNotationError(
            f"cannot send {character!r}: this game accepts only ASCII keys"
        )

    try:
        encoded = character.encode(encoding)
    except UnicodeEncodeError as error:
        raise KeyNotationError(
            f"cannot send {character!r}: it has no {encoding} representation"
        ) from error

    return "".join(encode_byte(value) for value in encoded)


def parse_keys(text: str, encoding: str | None = None) -> list[Key]:
    """LLM が書いたキー列を、1 回ずつ送るキーの並びに分ける。

    「<ESC>」のような名前に一致しない「<」は、そのまま「<」（上り階段）として扱う。
    ただし「<ESq>」「<ENTER」のように名前を書き間違えたとみなせるものは、残りの文字を
    1 つずつコマンドとして送ってしまわないよう、エラーにする。

    :param text: LLM が書いたキー列
    :param encoding: 全角文字を送るときのゲームの文字コード。None なら ASCII だけを受け付ける
    :return: 送るキーの並び
    """
    keys: list[Key] = []
    position = 0
    while position < len(text):
        character = text[position]
        match = NAME_PATTERN.match(text, position + 1) if character == "<" else None
        if match is not None:
            name = match.group()
            if text.startswith(">", match.end()):
                value = parse_named_key(name)
                if value is None:
                    raise KeyNotationError(
                        f"unknown key name <{name}>; use <ESC>, <ENTER>, <TAB>, <BS>, <SPACE>, "
                        "<C-x> (Ctrl+x), or <LT> / <GT> for a literal '<' / '>'"
                    )

                keys.append(Key(text[position : match.end() + 1], encode_byte(value)))
                position = match.end() + 1
                continue

            if UNCLOSED_NAME_PATTERN.match(name):
                raise KeyNotationError(
                    f"the key name <{name} has no closing '>'; close it (e.g. <ENTER>), "
                    "or write <LT> for the '<' key followed by other keys"
                )

        keys.append(Key(character, encode_character(character, encoding)))
        position += 1

    if len(keys) > MAX_KEYS_PER_CALL:
        raise KeyNotationError(
            f"too many keys ({len(keys)}); send at most {MAX_KEYS_PER_CALL} keys per call"
        )

    return keys


def join_text(keys: list[Key]) -> str:
    """キーの並びを、LLM が書いた表記に戻す。

    :param keys: キーの並び
    :return: 元の表記を連結した文字列
    """
    return "".join(key.text for key in keys)
