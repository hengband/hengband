"""制御サーバから得た画面と内部状態を、LLM に渡す観測にまとめる。

観測は 2 つに分けて返す。1 つは JSON にした要約 (入力待ちの種類、新しいメッセージ、
プレイヤーの状態、見えているモンスター等) で、もう 1 つは画面の文字そのもの
(地図の部分か画面全体) である。地図を JSON の文字列に入れると「\\」や「"」が
エスケープされて桁がずれて読みにくくなるため、画面はそのままのテキストで渡す。
"""

from dataclasses import dataclass
from typing import Any, Literal

#: 観測に含める画面の範囲
#: auto はコマンド入力待ちなら地図、それ以外 (メニューやプロンプト) なら画面全体
View = Literal["auto", "map", "screen", "none"]

#: 地図が描かれる画面上の範囲。src/io/cursor.cpp の panel_row_prt / panel_col_prt に合わせる
#: (地図の 1 行目は画面の 1 行目、1 桁目は画面の 13 桁目。最下行はステータス表示)
MAP_TOP = 1
MAP_LEFT = 13

#: Windows 版のフォント用の設定で、壁や鉱脈を塗りつぶした四角として描くための文字
WALL_CHARACTER = "\x7f"

#: 観測に含めるモンスターの数の上限
MAX_VISIBLE_MONSTERS = 15
MAX_DETECTED_MONSTERS = 10

#: モンスターの状態のうち、真のものを flags に並べる項目
MONSTER_FLAGS = (
    "asleep",
    "stunned",
    "confused",
    "fearful",
    "fast",
    "slow",
    "invulnerable",
    "friendly",
    "pet",
)

#: プレイヤーの状態異常のうち、真のものを status に並べる項目
PLAYER_STATUS_FLAGS = (
    "blind",
    "confused",
    "afraid",
    "poisoned",
    "paralyzed",
    "hallucinated",
)

#: 8 方向の名前と、その方向へ動くテンキー
DIRECTION_LABELS = {
    (-1, -1): "NW(7)",
    (0, -1): "N(8)",
    (1, -1): "NE(9)",
    (-1, 0): "W(4)",
    (1, 0): "E(6)",
    (-1, 1): "SW(1)",
    (0, 1): "S(2)",
    (1, 1): "SE(3)",
}


@dataclass(frozen=True)
class InputState:
    """ゲームが待っているキー入力の情報 (制御サーバの input op の結果)。"""

    kind: str
    message_sequence: int
    disturb_sequence: int
    character_generated: bool
    playing: bool
    dead: bool

    @classmethod
    def from_result(cls, result: dict[str, Any]) -> "InputState":
        """input op の result から作る。

        :param result: input op の result
        :return: 入力待ちの情報
        """
        return cls(
            kind=str(result.get("kind", "unknown")),
            message_sequence=int(result.get("message_sequence", 0)),
            disturb_sequence=int(result.get("disturb_sequence", 0)),
            character_generated=bool(result.get("character_generated", False)),
            playing=bool(result.get("playing", False)),
            dead=bool(result.get("dead", False)),
        )


def sign(value: int) -> int:
    """符号を返す。

    :param value: 値
    :return: 正なら 1、負なら -1、0 なら 0
    """
    return (value > 0) - (value < 0)


def direction_label(dx: int, dy: int) -> str:
    """プレイヤーから見た方向を 8 方向に丸めて、移動に使うテンキーと合わせて返す。

    縦横の比が 2:5 より偏っていれば縦か横、そうでなければ斜めとみなす
    (8 方向の境界である tan 22.5° ≒ 0.41 に近い比)。

    :param dx: 東向きを正とする横の差
    :param dy: 南向きを正とする縦の差
    :return: 「NE(9)」のような方向の名前。同じ位置なら「here」
    """
    if dx == 0 and dy == 0:
        return "here"

    step_x = sign(dx)
    step_y = sign(dy)
    if abs(dy) * 5 < abs(dx) * 2:
        step_y = 0
    elif abs(dx) * 5 < abs(dy) * 2:
        step_x = 0

    return DIRECTION_LABELS[(step_x, step_y)]


def distance(dx: int, dy: int) -> int:
    """ゲームと同じ式 (Grid::calc_distance) で距離を求める。

    :param dx: 横の差
    :param dy: 縦の差
    :return: 距離
    """
    ax = abs(dx)
    ay = abs(dy)
    return ay + ax // 2 if ay > ax else ax + ay // 2


def cell_width(character: str, encoding: str | None) -> int:
    """1 文字が画面で占める桁数を返す。

    日本語版の画面はゲームの文字コードの 1 バイトが 1 桁を占める
    (全角文字は 2 桁、Shift-JIS の半角カナは 1 桁)。

    :param character: 文字
    :param encoding: ゲームの文字コード。None なら英語版
    :return: 桁数
    """
    if ord(character) < 0x80 or encoding is None:
        return 1

    try:
        return len(character.encode(encoding))
    except UnicodeEncodeError:
        return 2


def to_cells(line: str, encoding: str | None) -> list[str | None]:
    """画面の 1 行を桁ごとに分ける。

    :param line: 制御サーバが返した 1 行 (UTF-8 に変換済み)
    :param encoding: ゲームの文字コード
    :return: 桁ごとの文字。全角文字の 2 桁目以降は None
    """
    cells: list[str | None] = []
    for character in line:
        cells.append(character)
        cells.extend([None] * (cell_width(character, encoding) - 1))

    return cells


def slice_cells(
    cells: list[str | None], start: int, end: int, encoding: str | None
) -> str:
    """桁の範囲を切り出して文字列にする。範囲の端で切れた全角文字は空白にする。

    :param cells: to_cells() で分けた 1 行
    :param start: 切り出す最初の桁
    :param end: 切り出す最後の桁の次
    :param encoding: ゲームの文字コード
    :return: 切り出した文字列
    """
    parts: list[str] = []
    for x in range(start, min(end, len(cells))):
        cell = cells[x]
        if cell is None:
            if x == start:
                parts.append(" ")

            continue

        if x + cell_width(cell, encoding) > end:
            parts.append(" ")
            continue

        parts.append(cell)

    return "".join(parts)


def screen_lines(screen: dict[str, Any] | None) -> list[str]:
    """screen op の結果から画面の各行を取り出す。

    ウィンドウを表示して動かした Windows 版は、壁や鉱脈を塗りつぶした四角で描くため
    (lib/pref/font-win.prf)、その文字 (0x7F) を # に置き換える。

    :param screen: screen op の結果。画面を読んでいない場合は None
    :return: 画面の各行
    """
    if not screen:
        return []

    return [str(line).replace(WALL_CHARACTER, "#") for line in screen.get("lines", [])]


def extract_map(lines: list[str], encoding: str | None) -> str:
    """画面から地図の部分を切り出す。

    左の能力値欄、最上行のメッセージ欄、最下行のステータス表示を除き、
    上下の空行と共通の字下げを取り除く。

    :param lines: 画面の各行
    :param encoding: ゲームの文字コード
    :return: 地図の各行を改行でつないだ文字列
    """
    rows: list[str] = []
    for line in lines[MAP_TOP : len(lines) - 1]:
        cells = to_cells(line, encoding)
        rows.append(slice_cells(cells, MAP_LEFT, len(cells), encoding).rstrip())

    while rows and not rows[0]:
        rows.pop(0)

    while rows and not rows[-1]:
        rows.pop()

    indent = min((len(row) - len(row.lstrip(" ")) for row in rows if row), default=0)
    return "\n".join(row[indent:] for row in rows)


def render_screen(lines: list[str]) -> str:
    """画面全体を、行末の空白を除いてテキストにする。

    :param lines: 画面の各行
    :return: 画面の各行を改行でつないだ文字列
    """
    return "\n".join(line.rstrip() for line in lines)


def summarize_player(snapshot: dict[str, Any]) -> dict[str, Any]:
    """内部状態からプレイヤーの要約を作る。

    :param snapshot: state op の result
    :return: プレイヤーの要約
    """
    player = snapshot.get("player", {})
    floor = snapshot.get("floor", {})
    summary: dict[str, Any] = {
        "level": player.get("level"),
        "hp": [player.get("hp"), player.get("max_hp")],
        "mp": [player.get("mp"), player.get("max_mp")],
        "exp": player.get("exp"),
        "exp_to_advance": player.get("exp_to_advance"),
        "gold": player.get("gold"),
        "ac": player.get("ac"),
        "speed": player.get("speed_display", {}).get("value"),
        "food": player.get("food_state"),
        "light": player.get("light_radius"),
        "place": floor.get("dungeon_name"),
        "depth": floor.get("level"),
        "pos": {"x": player.get("x"), "y": player.get("y")},
    }

    status_flags = player.get("status", {})
    status = [key for key in PLAYER_STATUS_FLAGS if status_flags.get(key)]
    if status_flags.get("stunned"):
        status.append(f"stun:{status_flags.get('stun_rank_name')}")

    if status_flags.get("cut"):
        status.append(f"cut:{status_flags.get('cut_rank_name')}")

    if status:
        summary["status"] = status

    effects = [entry.get("key") for entry in player.get("status_bar", [])]
    if effects:
        summary["effects"] = effects

    action = player.get("action", {})
    kind = action.get("kind", "none")
    if kind == "repeat":
        summary["action"] = f"repeat:{action.get('repeat_count')}"
    elif kind == "rest" and "rest" in action:
        summary["action"] = f"rest:{action['rest']}"
    elif kind != "none":
        summary["action"] = kind

    if player.get("recalling"):
        summary["recalling"] = True

    clock = snapshot.get("clock")
    if clock:
        day = clock.get("day")
        hour_minute = f"{clock.get('hour', 0):02d}:{clock.get('minute', 0):02d}"
        summary["clock"] = f"day {day} {hour_minute}" if day is not None else hour_minute

    return summary


def summarize_monsters(
    monsters: list[dict[str, Any]], player_x: int, player_y: int, limit: int
) -> tuple[list[dict[str, Any]], int]:
    """モンスターの一覧を、プレイヤーから近い順に要約する。

    :param monsters: state op の visible_monsters または detected_monsters
    :param player_x: プレイヤーの x 座標
    :param player_y: プレイヤーの y 座標
    :param limit: 返す数の上限
    :return: 要約したモンスターの一覧と、上限を超えて省いた数
    """
    entries: list[dict[str, Any]] = []
    for monster in monsters:
        dx = monster.get("x", player_x) - player_x
        dy = monster.get("y", player_y) - player_y
        name = "(hallucination)" if monster.get("hallucinated") else monster.get("name")
        entry: dict[str, Any] = {
            "name": name,
            "dir": direction_label(dx, dy),
            "dist": distance(dx, dy),
            "dx": dx,
            "dy": dy,
        }
        if monster.get("level") is not None:
            entry["level"] = monster["level"]

        health = monster.get("health")
        if health not in (None, "unhurt"):
            entry["health"] = health

        flags = [flag for flag in MONSTER_FLAGS if monster.get(flag)]
        if flags:
            entry["flags"] = flags

        entries.append(entry)

    entries.sort(key=lambda entry: entry["dist"])
    return entries[:limit], max(0, len(entries) - limit)


def summarize_items(snapshot: dict[str, Any]) -> dict[str, list[str]]:
    """持ち物と装備を 1 行ずつの文字列にする。

    :param snapshot: state op の result
    :return: inventory と equipment の一覧
    """
    inventory = []
    for item in snapshot.get("inventory", []):
        count = item.get("count", 1)
        suffix = f" x{count}" if count > 1 else ""
        inventory.append(f"{item.get('slot')}) {item.get('name')}{suffix}")

    equipment = [
        f"{item.get('slot')}: {item.get('name')}"
        for item in snapshot.get("equipment", [])
    ]
    return {"inventory": inventory, "equipment": equipment}


def choose_view(view: View, kind: str) -> View:
    """auto を実際の範囲に読み替える。

    :param view: 指定された範囲
    :param kind: ゲームが待っているキー入力の種類
    :return: 観測に含める画面の範囲
    """
    if view != "auto":
        return view

    return "map" if kind == "command" else "screen"


def build_observation(
    state: InputState,
    screen: dict[str, Any] | None,
    snapshot: dict[str, Any] | None,
    messages: list[str],
    messages_omitted: int,
    view: View,
    encoding: str | None,
    include_items: bool = False,
) -> tuple[dict[str, Any], str | None]:
    """観測を組み立てる。

    :param state: ゲームが待っているキー入力の情報
    :param screen: screen op の result。画面を使わない場合は None
    :param snapshot: state op の result。キャラクターが生成される前は None
    :param messages: 前回の観測以降に増えたメッセージ (古い順)
    :param messages_omitted: 上限を超えて省いたメッセージの数
    :param view: 観測に含める画面の範囲
    :param encoding: ゲームの文字コード
    :param include_items: 持ち物と装備を含めるか
    :return: JSON にする要約と、画面のテキスト (含めない場合は None)
    """
    summary: dict[str, Any] = {"input": state.kind}
    if state.dead:
        summary["dead"] = True

    lines = screen_lines(screen)
    if state.kind != "command" and lines and lines[0].strip():
        summary["prompt"] = lines[0].strip()

    summary["messages"] = messages
    if messages_omitted:
        summary["messages_omitted"] = messages_omitted

    if snapshot is not None:
        player = summarize_player(snapshot)
        summary["player"] = player
        player_x = player["pos"]["x"] or 0
        player_y = player["pos"]["y"] or 0
        monsters, omitted = summarize_monsters(
            snapshot.get("visible_monsters", []),
            player_x,
            player_y,
            MAX_VISIBLE_MONSTERS,
        )
        summary["monsters"] = monsters
        if omitted:
            summary["monsters_omitted"] = omitted

        detected, omitted = summarize_monsters(
            snapshot.get("detected_monsters", []),
            player_x,
            player_y,
            MAX_DETECTED_MONSTERS,
        )
        if detected:
            summary["detected_monsters"] = detected

        if omitted:
            summary["detected_monsters_omitted"] = omitted

        if include_items:
            summary.update(summarize_items(snapshot))

    actual_view = choose_view(view, state.kind)
    if actual_view == "none" or not lines:
        return summary, None

    summary["view"] = actual_view
    if actual_view == "map":
        return summary, extract_map(lines, encoding)

    return summary, render_screen(lines)
