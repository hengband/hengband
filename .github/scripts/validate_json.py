import hashlib
import re
import sys
from collections import OrderedDict
from collections.abc import Callable
from pathlib import Path
from urllib.parse import urljoin
import pyjson5
from pyjson5 import Json5Exception
from jsonschema import validate, ValidationError, SchemaError
from jsonschema.validators import validator_for
from referencing import Registry, Resource
from referencing.exceptions import Unresolvable
from referencing.jsonschema import specification_with


def load_jsonc(file_path: Path) -> dict:
    with file_path.open(encoding='utf-8-sig') as f:
        return pyjson5.load(f)


def load_all_schemas(schema_dir: Path) -> tuple[dict[Path, dict], dict[str, Path]]:
    schema_map: dict[str, Path] = {}
    loaded: dict[Path, dict] = {}
    errors: list[str] = []
    for s in sorted(schema_dir.rglob("*.schema.json")):
        # Nested schemas provide shared definitions, not lib/edit data types.
        if s.parent == schema_dir:
            base_name = s.stem.removesuffix(".schema")
            schema_map[base_name] = s
        try:
            schema_obj = load_jsonc(s)
            validator_for(schema_obj).check_schema(schema_obj)
            loaded[s] = schema_obj
        except (IOError, ValueError, SchemaError, Json5Exception) as e:
            errors.append(f"Error loading schema {s.name}: {e}")

    if not errors:
        return loaded, schema_map

    error_messages = ["Error: Failed to load schemas:"]
    error_messages.extend([f"- {err}" for err in errors])
    raise RuntimeError("\n".join(error_messages))


def build_schema_registry(loaded_schemas: dict[Path, dict]) -> Registry:
    """Register local schemas only; unresolved references must never use HTTP."""
    resources = []
    for path, schema in loaded_schemas.items():
        uri = path.resolve().as_uri()
        # Existing schemas have relative IDs (some end in .jsonc). Give each
        # resource an absolute base while retaining those IDs as local aliases.
        absolute_schema = schema if isinstance(schema, bool) else {**schema, "$id": urljoin(uri, schema.get("$id", uri))}
        specification = specification_with(validator_for(schema).META_SCHEMA["$id"])
        resource = Resource.from_contents(absolute_schema, default_specification=specification)
        resources.append((uri, resource))
        if not isinstance(absolute_schema, bool):
            resources.append((absolute_schema["$id"], resource))
    return Registry().with_resources(resources)


def build_validation_pairs(edit_dir: Path, schema_map: dict[str, Path], loaded_schemas: dict[Path, dict]) -> list[tuple[Path, Path, dict]]:
    pairs = []
    missing = []

    for data_file in sorted(edit_dir.glob("*.jsonc")):
        base_name = data_file.stem
        schema_file = schema_map.get(base_name)
        if not schema_file:
            missing.append(data_file.name)
            continue

        pairs.append((data_file, schema_file, loaded_schemas[schema_file]))

    # Per-quest files live one directory down and all share the single "Quest" schema.
    quests_dir = edit_dir / "quests"
    if quests_dir.is_dir():
        quest_schema_file = schema_map.get("Quest")
        if not quest_schema_file:
            raise RuntimeError("Missing schema: Quest.schema.json (required for lib/edit/quests/*.jsonc)")

        for data_file in sorted(quests_dir.glob("*.jsonc")):
            pairs.append((data_file, quest_schema_file, loaded_schemas[quest_schema_file]))

    # Town maps share one schema and live one directory down.
    towns_dir = edit_dir / "towns"
    if towns_dir.is_dir():
        town_map_schema_file = schema_map.get("TownMap")
        if not town_map_schema_file:
            raise RuntimeError("Missing schema: TownMap.schema.json (required for lib/edit/towns/*.jsonc)")

        for data_file in sorted(towns_dir.glob("*.jsonc")):
            pairs.append((data_file, town_map_schema_file, loaded_schemas[town_map_schema_file]))

    if missing:
        raise RuntimeError(f"Missing schemas for: {', '.join(missing)}")

    return pairs


def validate_vault_semantics(data: dict) -> None:
    """Check VaultReader constraints that draft-07 cannot express across fields.

    Call after schema validation, which checks required fields and container types.
    """
    previous_id = -1
    for index, vault in enumerate(data["vaults"]):
        path = ["vaults", index]
        for field in ("id", "type", "rating", "height", "width"):
            require_integer(vault[field], path + [field])
        if vault["id"] <= previous_id:
            raise ValidationError("IDs must be unique and in increasing order", path=path + ["id"])
        previous_id = vault["id"]
        if len(vault["layout"]) != vault["height"]:
            raise ValidationError("row count must equal height", path=path + ["layout"])
        for row_index, row in enumerate(vault["layout"]):
            row_path = path + ["layout", row_index]
            if any(ord(c) < 0x20 or ord(c) > 0x7e for c in row):
                raise ValidationError("layout must contain printable ASCII characters", path=row_path)
            if len(row) != vault["width"]:
                raise ValidationError("row byte length must equal width", path=row_path)


CPP_TOKEN_PATTERN = re.compile(r'^\s*\{\s*"([A-Z][A-Z0-9_]*)"\s*,', re.MULTILINE)


def load_cpp_tokens(path: Path) -> set[str]:
    tokens = set(CPP_TOKEN_PATTERN.findall(path.read_text(encoding="utf-8")))
    if not tokens:
        raise ValueError(f"No definition tokens found in {path}")
    return tokens


def validate_ego_semantics(data: dict, schema_path: Path) -> None:
    """Check unique IDs, strict integers and integer-percent probabilities."""
    repository_root = schema_path.resolve().parent.parent
    valid_flags = load_cpp_tokens(repository_root / "src/info-reader/baseitem-tokens-table.cpp")
    valid_activations = load_cpp_tokens(repository_root / "src/object-enchant/activation-info-table.cpp")
    require_integer(data["version"], ["version"])
    ids = set()
    integer_fields = ("id", "slot", "rating", "level", "rarity", "cost")
    for index, ego in enumerate(data["egos"]):
        path = ["egos", index]
        for field in integer_fields:
            require_integer(ego[field], path + [field])
        if ego["id"] in ids:
            raise ValidationError("IDs must be unique", path=path + ["id"])
        ids.add(ego["id"])
        if "activation" in ego and ego["activation"] not in valid_activations:
            raise ValidationError("unknown activation token", path=path + ["activation"])
        for flag_index, flag in enumerate(ego.get("flags", [])):
            if flag not in valid_flags:
                raise ValidationError("unknown ego flag token", path=path + ["flags", flag_index])
        for group in ("base_bonuses", "maximum_bonuses"):
            for field, value in ego.get(group, {}).items():
                require_integer(value, path + [group, field])
        for extra_index, extra in enumerate(ego.get("extra_flags", [])):
            for flag_index, flag in enumerate(extra["flags"]):
                if flag not in valid_flags:
                    raise ValidationError(
                        "unknown ego flag token",
                        path=path + ["extra_flags", extra_index, "flags", flag_index],
                    )
            require_integer(extra["chance"], path + ["extra_flags", extra_index, "chance"])


INT_MIN = -2147483648
INT_MAX = 2147483647


def require_reader_dice(value: str, path: list, minimum: int = INT_MIN, maximum: int = INT_MAX) -> None:
    # Dice::parse は「個数d面数」の両方をintの10進数として読み、符号は - だけを受け付ける。
    # pattern の $ は末尾の改行の前にも一致するため、文字列全体で確かめる。
    match = re.fullmatch(r"(-?)([0-9]+)d(-?)([0-9]+)", value)
    if not match:
        raise ValidationError("dice must be written as XdY", path=path)
    for sign, digits in (match.group(1, 2), match.group(3, 4)):
        # 先頭の0を除いてから桁数を見て、Pythonの整数変換の桁数の上限に当たらないようにする。
        digits = digits.lstrip("0") or "0"
        if len(digits) > len(str(INT_MAX)) or not INT_MIN <= int(sign + digits) <= INT_MAX:
            raise ValidationError("dice values must fit in int", path=path)
        if not minimum <= int(sign + digits) <= maximum:
            raise ValidationError(f"dice values must be between {minimum} and {maximum}", path=path)


# セーブではアイテムのダイスの個数・面数をそれぞれ1バイトで書くため、BaseitemReader・ArtifactReaderは0〜255に限る。
ITEM_DICE_RANGE = (0, 255)


INVALID_BASEITEM_NAMES = {"ja": "異常アイテム", "en": "Invalid item"}


def load_rod_type_value(repository_root: Path) -> int:
    """ItemKindType::ROD の値を読む。"""
    path = repository_root / "src/object/tval-types.h"
    match = re.search(r'^\s*ROD\s*=\s*(\d+)\s*,', path.read_text(encoding="utf-8"), re.MULTILINE)
    if not match:
        raise ValueError(f"ItemKindType::ROD not found in {path}")
    return int(match[1])


def validate_baseitem_semantics(data: dict, schema_path: Path) -> None:
    """BaseitemReaderの整数型・ID順序・表示・ロッドのpval・ダイス・自然生成と生成情報の併記を検証する。"""
    repository_root = schema_path.resolve().parent.parent
    valid_colors = load_cpp_map_keys(repository_root / "src/term/gameterm.cpp", "color_list")
    rod_type_value = load_rod_type_value(repository_root)
    valid_keys = {language: set() for language in INVALID_BASEITEM_NAMES}
    previous_id = -1
    for index, baseitem in enumerate(data["baseitems"]):
        path = ["baseitems", index]
        for field in ("id", "parameter_value", "level", "weight", "cost", "base_ac", "hit_bonus", "damage_bonus", "ac_bonus"):
            if field in baseitem:
                require_integer(baseitem[field], path + [field])
        for field in ("type_value", "subtype_value"):
            require_integer(baseitem["itemkind"][field], path + ["itemkind", field])
        if baseitem["id"] <= previous_id:
            raise ValidationError("baseitem IDs must be strictly increasing", path=path + ["id"])
        previous_id = baseitem["id"]

        # BaseitemReaderは表示文字を1バイトちょうどに限る。
        if len(baseitem["symbol"]["character"].encode("utf-8")) != 1:
            raise ValidationError("symbol character must be a single byte", path=path + ["symbol", "character"])
        if baseitem["symbol"]["color"] not in valid_colors:
            raise ValidationError("unknown color", path=path + ["symbol", "color"])
        # BaseitemReaderはロッドのpval(省略時は0)が正でなければ拒否する。
        if baseitem["itemkind"]["type_value"] == rod_type_value and baseitem.get("parameter_value", 0) <= 0:
            raise ValidationError("rod parameter_value must be positive", path=path + ["parameter_value"])
        if "base_dice" in baseitem:
            require_reader_dice(baseitem["base_dice"], path + ["base_dice"], *ITEM_DICE_RANGE)
        for allocation_index, allocation in enumerate(baseitem.get("allocations", [])):
            for field in ("depth", "rarity"):
                require_integer(allocation[field], path + ["allocations", allocation_index, field])
        # 自然生成しないベースアイテムは生成テーブルを持たないため、BaseitemReaderは
        # natural_generation: false と空でない allocations の併記を拒否する。空の allocations は書かないのと同じ。
        if baseitem.get("natural_generation", True) is False and baseitem.get("allocations"):
            raise ValidationError("baseitem without natural generation must not have allocations", path=path + ["allocations"])
        # 種別・副種別から ID を引く表では後の ID が先の ID を隠すため、BaseitemReaderは
        # 有効なベースアイテムどうしの重複を拒否する。有効かどうかはビルドの言語の名前で決まる。
        key = (baseitem["itemkind"]["type_value"], baseitem["itemkind"]["subtype_value"])
        for language, invalid in INVALID_BASEITEM_NAMES.items():
            if baseitem["name"][language] in ("", invalid):
                continue
            if key in valid_keys[language]:
                raise ValidationError("kind of a valid baseitem must be unique", path=path + ["itemkind"])
            valid_keys[language].add(key)


def collect_valid_baseitem_keys(baseitem_data: dict) -> set[tuple[int, int]]:
    """日英どちらのビルドでも、有効なベースアイテムがある (種別, 副種別) を集める。"""
    # BaseitemDefinition::is_valid() は、ビルドの言語の名前が空でも「異常アイテム」でもないものを有効とする。
    # 言語ごとに有効な種別を集めて両方にあるものを返す（同じ種別の別の項目が、それぞれの言語で有効でもよい）。
    keys_by_language = [
        {
            (baseitem["itemkind"]["type_value"], baseitem["itemkind"]["subtype_value"])
            for baseitem in baseitem_data["baseitems"]
            if baseitem["name"][language] not in ("", invalid)
        }
        for language, invalid in INVALID_BASEITEM_NAMES.items()
    ]
    return set.intersection(*keys_by_language)


def load_valid_baseitem_keys(schema_path: Path, data_path: Path, registry: Registry) -> set[tuple[int, int]]:
    """アーティファクトの検証には隣の候補ベースアイテム定義を優先し、なければ同梱定義を使う。"""
    baseitem_data = load_validated_companion(schema_path, data_path, registry, "BaseitemDefinitions", "baseitem",
                                             lambda data: validate_baseitem_semantics(data, schema_path))
    return collect_valid_baseitem_keys(baseitem_data)


def validate_artifact_semantics(data: dict, valid_baseitem_keys: set[tuple[int, int]] | None = None) -> None:
    """ArtifactReaderの整数型・ID順序・ダイス・元のベースアイテムを検証する。"""
    for index, artifact in enumerate(data["artifacts"]):
        path = ["artifacts", index]
        for field in ("id", "parameter_value", "level", "rarity", "weight", "cost", "base_ac", "hit_bonus", "damage_bonus", "ac_bonus"):
            if field in artifact:
                require_integer(artifact[field], path + [field])
        for field in ("type_value", "subtype_value"):
            require_integer(artifact["base_item"][field], path + ["base_item", field])
        # 実行時の記録は 1〜件数の ID だけを作るため、ArtifactReaderは 1 から欠番なく並ぶことを求める。
        if artifact["id"] != index + 1:
            raise ValidationError("artifact IDs must start at 1 and continue without gaps", path=path + ["id"])
        if "base_dice" in artifact:
            require_reader_dice(artifact["base_dice"], path + ["base_dice"], *ITEM_DICE_RANGE)
        # ArtifactReaderは、元のベースアイテムが有効なベースアイテムに無ければ拒否する。
        key = (artifact["base_item"]["type_value"], artifact["base_item"]["subtype_value"])
        if valid_baseitem_keys is not None and key not in valid_baseitem_keys:
            raise ValidationError("base item must be a valid baseitem", path=path + ["base_item"])


# TownList::is_all_initialized() が名前を求める町（1〜VALID_TOWNS-1）。町1には別名も要る。
REQUIRED_TOWN_IDS = range(1, 6)


def count_town_files(repository_root: Path) -> int:
    """TownList と同じく、町の定義ファイル名の先頭2文字（町の番号）の種類を数える。"""
    return len({path.name[:2] for path in (repository_root / "lib/edit/towns").glob("*.jsonc")})


def validate_wilderness_semantics(data: dict, schema_path: Path | None = None) -> None:
    """Check cross-field constraints used by WildernessReader."""
    for field in ("version", "width", "height"):
        if type(data[field]) is not int:
            raise ValidationError("expected an integer JSON value", path=[field])

    town_ids = set()
    town_file_count = count_town_files(schema_path.resolve().parent.parent) if schema_path is not None else None
    for index, town in enumerate(data["towns"]):
        if type(town["id"]) is not int:
            raise ValidationError("expected an integer JSON value", path=["towns", index, "id"])
        if town["id"] in town_ids:
            raise ValidationError("town IDs must be unique", path=["towns", index, "id"])
        town_ids.add(town["id"])
        # 初期化は町の定義ファイルより後の番号の町を拒否して終了する。
        if town_file_count is not None and town["id"] > town_file_count:
            raise ValidationError("town ID must have a town definition file", path=["towns", index, "id"])
    for town_id in REQUIRED_TOWN_IDS:
        if town_id not in town_ids:
            raise ValidationError(f"town {town_id} must be defined", path=["towns"])
    if not any(town["id"] == 1 and "alias" in town for town in data["towns"]):
        raise ValidationError("town 1 must have an alias", path=["towns"])

    width = data["width"]
    height = data["height"]
    for map_name, wilderness_map in data["maps"].items():
        map_path = ["maps", map_name]
        symbols = set()
        for index, letter in enumerate(wilderness_map["letters"]):
            for field in ("terrain", "town", "road"):
                if field in letter and type(letter[field]) is not int:
                    raise ValidationError("expected an integer JSON value", path=map_path + ["letters", index, field])
            if "level" in letter:
                levels = letter["level"].values() if isinstance(letter["level"], dict) else (letter["level"],)
                if any(type(level) is not int for level in levels):
                    raise ValidationError("expected an integer JSON value", path=map_path + ["letters", index, "level"])
            if letter.get("town", 0) != 0 and letter["town"] not in town_ids:
                raise ValidationError("letter references an undefined town ID", path=map_path + ["letters", index, "town"])
            symbol = letter["symbol"]
            if any(ord(c) < 0x20 or ord(c) > 0x7e for c in symbol):
                raise ValidationError("symbol must be printable ASCII", path=map_path + ["letters", index, "symbol"])
            if symbol in symbols:
                raise ValidationError("symbols must be unique within a map", path=map_path + ["letters", index, "symbol"])
            symbols.add(symbol)

        layout = wilderness_map["layout"]
        if len(layout) > height:
            raise ValidationError("row count must not exceed height", path=map_path + ["layout"])
        if map_name == "normal" and len(layout) != height:
            raise ValidationError("normal row count must equal height", path=map_path + ["layout"])
        letters_by_symbol = {letter["symbol"]: letter for letter in wilderness_map["letters"]}
        for row_index, row in enumerate(layout):
            if len(row) > width or (map_name == "normal" and len(row) != width):
                raise ValidationError("normal row width must equal width" if map_name == "normal" else "row width must not exceed width", path=map_path + ["layout", row_index])
            for column, symbol in enumerate(row):
                if symbol not in symbols:
                    raise ValidationError("layout contains an undefined symbol", path=map_path + ["layout", row_index, column])
                # 外周が通れると、荒野の端の区画に入ったときに範囲外を引いて落ちるため、WildernessReaderは
                # 外周を外周の地形（terrain 0）で町も道も無いマスに限る。レイアウトに無いマスは外周の地形になる。
                if row_index in (0, height - 1) or column in (0, width - 1):
                    letter = letters_by_symbol[symbol]
                    if letter["terrain"] != 0 or letter.get("town", 0) != 0 or letter.get("road", 0) != 0:
                        raise ValidationError("wilderness edge must be impassable edge terrain without town or road",
                                              path=map_path + ["layout", row_index, column])

        position = wilderness_map["starting_position"]
        for field in ("x", "y"):
            if type(position[field]) is not int:
                raise ValidationError("expected an integer JSON value", path=map_path + ["starting_position", field])
        # 地上の生成では現在地の周囲8マスを生成するため、WildernessReaderは荒野の端を拒否する。
        if position["x"] > width - 2 or position["y"] > height - 2:
            raise ValidationError("starting position must not be on the wilderness edge", path=map_path + ["starting_position"])
        if position["y"] >= len(layout) or position["x"] >= len(layout[position["y"]]):
            raise ValidationError("starting position must be within the map layout", path=map_path + ["starting_position"])


def validate_quest_semantics(data: dict) -> None:
    """Reject integral floats such as 5.0 and legend symbols which QuestReader rejects."""
    # Quest・QuestPreferences の schema に number 型の項目は無く、数値はすべて整数として読む。
    def walk(value, path):
        if isinstance(value, float):
            raise ValidationError("expected an integer JSON value", path=path)
        if isinstance(value, dict):
            for key, child in value.items():
                walk(child, path + [key])
        elif isinstance(value, list):
            for index, child in enumerate(value):
                walk(child, path + [index])

    walk(data, [])
    for symbol in data.get("legend", {}):
        # propertyNames の maxLength は文字数で数えるため、QuestReaderと同じく印字可能なASCIIの1文字ちょうどを確かめる。
        if len(symbol) != 1 or not is_fixed_map_symbol(symbol):
            raise ValidationError("legend symbol must be exactly one printable ASCII character", path=["legend", symbol])


def is_fixed_map_symbol(character: str) -> bool:
    """固定マップの記号（印字可能なASCII）か。general-parser.h の is_fixed_map_symbol と揃える。"""
    return " " <= character <= "~"


def select_quest_maps(data: dict) -> list[tuple[list, list[str]]]:
    """QuestReaderが使う地図を、診断用の位置と組で返す。map があれば mapVariants は読まない。"""
    if "map" in data:
        return [(["map"], data["map"])]
    return [(["mapVariants", index], rows) for index, rows in enumerate(data.get("mapVariants", []))]


def select_quest_starts(data: dict) -> list[tuple[list, dict]]:
    """QuestReaderが使う開始位置を、診断用の位置と組で返す。start があれば startVariants は読まない。"""
    if "start" in data:
        return [(["start"], data["start"])]
    return [(["startVariants", index], start) for index, start in enumerate(data.get("startVariants", []))]


def validate_quest_layout_semantics(data: dict) -> None:
    """QuestReaderと同じく、地図の行と開始位置を検証する。"""
    maps = select_quest_maps(data)
    for map_path, rows in maps:
        # 行数と行の長さの上限は schema が持つ。行の長さは文字数で数えるため、印字可能なASCIIに限ってバイト数と揃える。
        for row_index, row in enumerate(rows):
            for column, character in enumerate(row):
                if not is_fixed_map_symbol(character):
                    raise ValidationError("map rows must contain printable ASCII characters", path=map_path + [row_index, column])
            # 生成側は最終行の長さからフロア幅を決めるため、QuestReaderは行幅が揃わない地図を拒否する。
            if len(row) != len(rows[0]):
                raise ValidationError("map rows must have the same width", path=map_path + [row_index])
    for start_path, start in select_quest_starts(data):
        # 開始位置は条件に関係なく、すべての地図の候補の中に収まる必要がある。
        if not maps or not all(start["y"] < len(rows) and start["x"] < len(rows[0]) for _, rows in maps):
            raise ValidationError("start position must be within every map variant", path=start_path)


def extract_artifact_ids(data: dict) -> frozenset[int]:
    for index, artifact in enumerate(data["artifacts"]):
        require_integer(artifact["id"], ["artifacts", index, "id"])
    return frozenset(artifact["id"] for artifact in data["artifacts"])


def extract_dungeon_ids(data: dict) -> frozenset[int]:
    for index, dungeon in enumerate(data["dungeons"]):
        require_integer(dungeon["id"], ["dungeons", index, "id"])
    return frozenset(dungeon["id"] for dungeon in data["dungeons"])


def extract_valid_baseitem_ids(data: dict) -> frozenset[int]:
    """日英どちらのビルドでも BaseitemDefinition::is_valid() が真になるベースアイテムのIDを集める。"""
    # is_valid() は、ビルドの言語の名前が空でも「異常アイテム」でもないものを有効とする。
    invalid_names = {"ja": "異常アイテム", "en": "Invalid item"}
    for index, baseitem in enumerate(data["baseitems"]):
        require_integer(baseitem["id"], ["baseitems", index, "id"])
    return frozenset(baseitem["id"] for baseitem in data["baseitems"]
                     if all(baseitem["name"][language] not in ("", invalid) for language, invalid in invalid_names.items()))


def extract_terrain_tags(data: dict) -> frozenset[str]:
    return frozenset(terrain["key"] for terrain in data["terrains"])


def quest_companion_anchor(data_path: Path) -> Path:
    """付随定義を探す基準のパスを返す（find_companion_path はこのパスと同じディレクトリを探す）。"""
    # クエストは lib/edit/quests/ に置かれるため、付随定義は親の lib/edit の候補を優先し、なければ同梱定義を使う。
    return data_path.parent if data_path.parent.name == "quests" else data_path


def load_quest_references(schema_path: Path, data_path: Path, registry: Registry) -> dict[str, frozenset]:
    """QuestListが照合する定義のID集合を返す。"""
    anchor = quest_companion_anchor(data_path)
    return {
        "monster": load_companion_ids(schema_path, anchor, registry, "MonraceDefinitions", "monster", extract_monster_ids),
        "object": load_companion_ids(schema_path, anchor, registry, "BaseitemDefinitions", "baseitem", extract_valid_baseitem_ids),
        "artifact": load_companion_ids(schema_path, anchor, registry, "ArtifactDefinitions", "artifact", extract_artifact_ids),
        "dungeon": load_companion_ids(schema_path, anchor, registry, "DungeonDefinitions", "dungeon", extract_dungeon_ids),
        "terrain": load_companion_ids(schema_path, anchor, registry, "TerrainDefinitions", "terrain", extract_terrain_tags),
    }


def extract_quest_preference_symbols(data: dict) -> frozenset[str]:
    validate_quest_semantics(data)
    return frozenset(data["legend"])


def find_undefined_map_symbol(maps: list[tuple[list, list[str]]], *legends) -> tuple[list, str] | None:
    """どの凡例にも無い最初の記号を、診断用の位置と組で返す。"""
    for map_path, rows in maps:
        for row_index, row in enumerate(rows):
            for column, symbol in enumerate(row):
                if not any(symbol in legend for legend in legends):
                    return map_path + [row_index, column], symbol
    return None


def validate_quest_map_symbols(data: dict, schema_path: Path, data_path: Path, registry: Registry) -> None:
    """QuestListと同じく、地図の記号がクエストの凡例か共通凡例にあることを確かめる。"""
    # 生成時はどちらにも無い記号の凡例を上書きしないため、前に生成した町やクエストの凡例がそのまま使われる。
    base_symbols = load_companion_ids(schema_path, quest_companion_anchor(data_path), registry,
                                      "QuestPreferences", "quest preference",
                                      extract_quest_preference_symbols)
    undefined = find_undefined_map_symbol(select_quest_maps(data), data.get("legend", {}), base_symbols)
    if undefined is not None:
        raise ValidationError("map symbol is defined neither in the quest legend nor in the shared legend", path=undefined[0])


def validate_quest_preference_map_symbols(data: dict, data_path: Path) -> None:
    """共通凡例から記号を消しても、それを使うクエストの地図が CI で分かるようにする。"""
    # 共通凡例を使うのは同じ lib/edit の quests/ のクエストだけなので、隣に quests/ が無ければ照合しない。
    quests_dir = data_path.parent / "quests"
    if not quests_dir.is_dir():
        return
    for quest_path in sorted(quests_dir.glob("*.jsonc")):
        quest = load_sibling_quest(quest_path)
        # 読めない・形の壊れたクエストは、そのファイル自身の検証で拒否される。
        if not isinstance(quest, dict) or not isinstance(quest.get("legend", {}), dict):
            continue
        if not isinstance(quest.get("map", []), list) or not isinstance(quest.get("mapVariants", []), list):
            continue
        maps = [(path, rows) for path, rows in select_quest_maps(quest)
                if isinstance(rows, list) and all(isinstance(row, str) for row in rows)]
        undefined = find_undefined_map_symbol(maps, quest.get("legend", {}), data["legend"])
        if undefined is not None:
            raise ValidationError(f"map symbol {undefined[1]!r} at {undefined[0]} in {quest_path.name} "
                                  "is defined neither in its legend nor in the shared legend", path=["legend"])


def load_sibling_quest(quest_path: Path):
    """隣のクエストを読む。読めないファイルは None を返し、そのファイル自身の検証で拒否させる。"""
    try:
        return load_jsonc(quest_path)
    except (OSError, ValueError, Json5Exception):
        return None


def load_quest_ids(quests_dir: Path) -> frozenset[int]:
    """QuestListと同じく、クエストのディレクトリにある全 .jsonc のIDを集める。"""
    ids = set()
    for quest_path in sorted(quests_dir.glob("*.jsonc")):
        quest = load_sibling_quest(quest_path)
        # 読めない・形の壊れたクエストは、そのファイル自身の検証で拒否される。
        if isinstance(quest, dict) and type(quest.get("id")) is int:
            ids.add(quest["id"])
    return frozenset(ids)


def validate_quest_legend_references(legend: dict, references: dict[str, frozenset]) -> None:
    """QuestListと同じく、凡例の地形と固定の参照を照合する。"""
    # ランダム生成の深さ補正と受託時の報酬指定は固定IDではない。ランダムなアーティファクトの id は固定の参照として使われる。
    # ego は配置で使われないため照合しない。special は配置後の地形で意味が決まるため、起動時の照合に任せる。
    for symbol, cell in legend.items():
        path = ["legend", symbol]
        if cell.get("terrain", "*") != "*" and cell["terrain"] not in references["terrain"]:
            raise ValidationError("unknown terrain tag", path=path + ["terrain"])
        if isinstance(cell.get("trap"), str) and cell["trap"] not in references["terrain"]:
            raise ValidationError("unknown terrain tag", path=path + ["trap"])
        # 整数で書いた固定IDと、オブジェクトの中のIDを (位置, 種類, 値, 理由) で並べる。
        fixed_references = []
        for field, kind, nested, reason in (("monster", "monster", "cloneOf", "unknown monster ID"),
                                            ("object", "object", None, "unknown or invalid baseitem ID"),
                                            ("artifact", "artifact", "id", "unknown artifact ID")):
            value = cell.get(field)
            if type(value) is int:
                fixed_references.append((path + [field], kind, value, reason))
            elif isinstance(value, dict) and nested is not None and nested in value:
                fixed_references.append((path + [field, nested], kind, value[nested], reason))
        for reference_path, kind, value, reason in fixed_references:
            if value not in references[kind]:
                raise ValidationError(reason, path=reference_path)


def validate_quest_references(data: dict, schema_path: Path, data_path: Path, registry: Registry) -> None:
    """QuestListと同じく、対象モンスター・ダンジョン・報酬・退出元のクエスト・凡例の参照を照合する。"""
    references = load_quest_references(schema_path, data_path, registry)
    definition = data["definition"]
    # 対象モンスターは省略時も 0 として照合される。0 以外の dungeon だけを照合する（0 は指定なし）。
    if definition.get("monster", 0) not in references["monster"]:
        raise ValidationError("unknown monster ID", path=["definition", "monster"])
    if definition.get("dungeon", 0) != 0 and definition["dungeon"] not in references["dungeon"]:
        raise ValidationError("unknown dungeon ID", path=["definition", "dungeon"])
    reward = definition.get("reward", {})
    if "artifact" in reward and reward["artifact"] not in references["artifact"]:
        raise ValidationError("unknown artifact ID", path=["definition", "reward", "artifact"])
    for index, candidate in enumerate(reward.get("artifacts", [])):
        if candidate not in references["artifact"]:
            raise ValidationError("unknown artifact ID", path=["definition", "reward", "artifacts", index])
    quest_ids = None
    for start_path, start in select_quest_starts(data):
        if start.get("leavingQuest", 0) == 0:
            continue
        if quest_ids is None:
            quest_ids = load_quest_ids(data_path.parent)
        if start["leavingQuest"] not in quest_ids:
            raise ValidationError("unknown quest ID", path=start_path + ["leavingQuest"])
    validate_quest_legend_references(data.get("legend", {}), references)


def validate_quest_file_semantics(data: dict, schema_path: Path, data_path: Path, registry: Registry) -> None:
    validate_quest_semantics(data)
    validate_quest_layout_semantics(data)
    validate_quest_references(data, schema_path, data_path, registry)
    validate_quest_map_symbols(data, schema_path, data_path, registry)


def validate_quest_preferences_semantics(data: dict, schema_path: Path, data_path: Path, registry: Registry) -> None:
    validate_quest_semantics(data)
    validate_quest_legend_references(data["legend"], load_quest_references(schema_path, data_path, registry))
    validate_quest_preference_map_symbols(data, data_path)


def validate_town_preferences_semantics(data: dict, schema_path: Path) -> None:
    """Check town legend values against the terrain definitions and reader types."""
    repository_root = schema_path.resolve().parent.parent
    terrain_data = load_jsonc(repository_root / "lib/edit/TerrainDefinitions.jsonc")
    valid_terrain_tags = {terrain["key"] for terrain in terrain_data["terrains"]}

    if type(data["version"]) is not int:
        raise ValidationError("expected an integer JSON value", path=["version"])
    for symbol, cell in data["legend"].items():
        # propertyNames の $ は末尾の改行の前にも一致するため、TownPreferencesReaderと同じく1文字ちょうどを確かめる。
        if len(symbol) != 1:
            raise ValidationError("legend symbol must be exactly one printable ASCII character", path=["legend", symbol])
        terrain_tag = cell["terrain"]
        if terrain_tag != "*" and terrain_tag not in valid_terrain_tags:
            raise ValidationError("unknown terrain tag", path=["legend", symbol, "terrain"])
        if "special" in cell and type(cell["special"]) is not int:
            raise ValidationError("expected an integer JSON value", path=["legend", symbol, "special"])


# 荒野の地図に置かない旧海底都市（TownId::RLYEH）の番号。町の定義の一覧には書いてよい。
# C++ の SECRET_TOWN（町の番号5）とは別の町なので、値を揃えない。
RLYEH_TOWN_NUMBER = 6


def validate_town_definition_list_semantics(data: dict, schema_path: Path, data_path: Path | None = None,
                                            registry: Registry | None = None) -> None:
    """Check town map references against the distributed map files and the wilderness towns."""
    if type(data["version"]) is not int:
        raise ValidationError("expected an integer JSON value", path=["version"])

    if data_path is not None:
        # TownDefinitionListReaderは今いる町が一覧に無いと読み込みを失敗させるため、荒野の町はすべて一覧に要る。
        # 一覧のキーは、荒野の町か旧海底都市に限る（ほかのキーは使われない）。
        wilderness = load_validated_companion(schema_path, data_path, registry, "WildernessDefinition", "wilderness",
                                              lambda companion: validate_wilderness_semantics(companion, schema_path))
        town_ids = {town["id"] for town in wilderness["towns"]}
        listed = {int(town) for town in data["towns"]}
        missing = sorted(town_ids - listed)
        if missing:
            raise ValidationError(f"wilderness towns {missing} must have a town map", path=["towns"])
        for town in data["towns"]:
            if int(town) not in town_ids | {RLYEH_TOWN_NUMBER}:
                raise ValidationError("town must be a wilderness town or the obsoleted R'lyeh town", path=["towns", town])

    edit_dir = schema_path.resolve().parent.parent / "lib/edit"
    for town, entry in data["towns"].items():
        maps = entry.items() if isinstance(entry, dict) else ((None, entry),)
        for mode, map_file in maps:
            if not (edit_dir / map_file).is_file():
                path = ["towns", town] + ([mode] if mode else [])
                raise ValidationError("town map file does not exist", path=path)


def validate_town_map_semantics(data: dict, schema_path: Path) -> None:
    """Check town map dimensions and starting coordinates used by the map reader."""
    # Keep these limits in sync with MAX_HGT / MAX_WID in
    # src/floor/floor-base-definitions.h. Town maps are loaded with those bounds.
    max_height = 66
    max_width = 198
    if type(data["version"]) is not int:
        raise ValidationError("expected an integer JSON value", path=["version"])
    if data["version"] != 2:
        raise ValidationError("expected version 2", path=["version"])

    repository_root = schema_path.resolve().parent.parent
    terrain_data = load_jsonc(repository_root / "lib/edit/TerrainDefinitions.jsonc")
    valid_terrain_tags = {terrain["key"] for terrain in terrain_data["terrains"]}
    feature_tokens = {
        "monster": r"(?:-?[0-9]+|\*[0-9]*|c[0-9]+)",
        "object": r"(?:-?[0-9]+|\*[0-9]*|!)",
        "ego": r"(?:-?[0-9]+|\*[0-9]*)",
        "artifact": r"(?:-?[0-9]+|\*[0-9]*|!)",
    }
    for rule_index, rule in enumerate(data["featureRules"]):
        # pattern の $ は末尾の改行の前にも一致するため、TownMapReaderと同じく1文字ちょうどを確かめる。
        if len(rule["symbol"]) != 1:
            raise ValidationError("feature symbol must be exactly one printable ASCII character", path=["featureRules", rule_index, "symbol"])
        definition = rule["definition"]
        for field in ("terrain", "monster", "object", "ego", "artifact", "trap"):
            if any(not (" " <= character <= "~") for character in definition[field]):
                raise ValidationError("feature field must contain printable ASCII characters", path=["featureRules", rule_index, "definition", field])
            if any(delimiter in definition[field] for delimiter in (":", "/", "\\", "\r", "\n")):
                raise ValidationError("feature field contains a legacy delimiter or line break", path=["featureRules", rule_index, "definition", field])
        for field in ("caveInfo", "special"):
            if type(definition[field]) is not int:
                raise ValidationError("expected an integer JSON value", path=["featureRules", rule_index, "definition", field])
        for field in ("terrain", "trap"):
            terrain_tag = definition[field]
            if terrain_tag != "*" and terrain_tag not in valid_terrain_tags:
                raise ValidationError("unknown terrain tag", path=["featureRules", rule_index, "definition", field])
        for field, pattern in feature_tokens.items():
            token = definition[field]
            if re.fullmatch(pattern, token) is None:
                raise ValidationError("invalid fixed-map token", path=["featureRules", rule_index, "definition", field])
            number = token.removeprefix("*").removeprefix("c")
            if re.fullmatch(r"-?[0-9]+", number):
                integer = int(number)
                if field in ("monster", "object", "artifact"):
                    if token.startswith("c"):
                        minimum, maximum = 0, 32767
                    elif token.startswith("*"):
                        minimum, maximum = 0, 32767
                    elif field == "monster":
                        minimum, maximum = -32767, 32767
                    else:
                        minimum, maximum = -32768, 32767
                else:
                    minimum, maximum = -2147483648, 2147483647
                if integer < minimum or integer > maximum:
                    raise ValidationError("fixed-map token exceeds the reader's integer range", path=["featureRules", rule_index, "definition", field])

    for rule_index, rule in enumerate(data["buildingRules"]):
        if type(rule["index"]) is not int:
            raise ValidationError("expected an integer JSON value", path=["buildingRules", rule_index, "index"])
        fields = rule["fields"]
        for field_index, value in enumerate(fields):
            if ":" in value or "/" in value or "\\" in value or "\r" in value or "\n" in value:
                raise ValidationError("building fields must not contain legacy delimiters or line breaks", path=["buildingRules", rule_index, "fields", field_index])
        if rule["command"] == "N" and len(fields) != 3:
            raise ValidationError("building name command requires three fields", path=["buildingRules", rule_index, "fields"])
        if rule["command"] == "A":
            if len(fields) != 7 or any(re.fullmatch(r"-?[0-9]+", fields[index]) is None for index in (0, 2, 3, 5, 6)):
                raise ValidationError("building action command requires seven valid fields", path=["buildingRules", rule_index, "fields"])
            for field_index in (0, 2, 3, 5, 6):
                minimum, maximum = (-32768, 32767) if field_index in (5, 6) else (-2147483648, 2147483647)
                if not minimum <= int(fields[field_index]) <= maximum:
                    raise ValidationError("building field exceeds the reader's integer range", path=["buildingRules", rule_index, "fields", field_index])
            if not 0 <= int(fields[0]) < 8:
                raise ValidationError("building action index is outside the reader's range", path=["buildingRules", rule_index, "fields", 0])
        if rule["command"] in ("C", "M", "R") and not fields:
            raise ValidationError("building membership command requires at least one field", path=["buildingRules", rule_index, "fields"])
        if rule["command"] in ("C", "M", "R") and any(re.fullmatch(r"-?[0-9]+", value) is None for value in fields):
            raise ValidationError("building membership fields must be integers", path=["buildingRules", rule_index, "fields"])
        if rule["command"] in ("C", "M", "R"):
            max_fields = {"C": 29, "M": 10, "R": 38}[rule["command"]]
            if len(fields) > max_fields:
                raise ValidationError("too many fields for building membership command", path=["buildingRules", rule_index, "fields"])
            for field_index, value in enumerate(fields):
                if not -32768 <= int(value) <= 32767:
                    raise ValidationError("building field exceeds the reader's integer range", path=["buildingRules", rule_index, "fields", field_index])

    map_dimensions = []
    require_conditions = len(data["mapVariants"]) > 1
    for map_index, variant in enumerate(data["mapVariants"]):
        if require_conditions and "when" not in variant:
            raise ValidationError("every map variant requires when when multiple variants are defined", path=["mapVariants", map_index])
        rows = variant["rows"]
        if len(rows) > max_height:
            raise ValidationError("map height exceeds the reader's maximum", path=["mapVariants", map_index, "rows"])
        width = len(rows[0])
        if width > max_width:
            raise ValidationError("map width exceeds the reader's maximum", path=["mapVariants", map_index, "rows", 0])
        for row_index, row in enumerate(rows):
            # pattern の $ は末尾の改行の前にも一致するため、TownMapReaderと同じく行全体を確かめる。
            # 末尾の改行で幅が食い違ったときに原因を指すよう、幅より先に確かめる。
            if any(not " " <= character <= "~" for character in row):
                raise ValidationError("map rows must contain printable ASCII characters", path=["mapVariants", map_index, "rows", row_index])
            if len(row) != width:
                raise ValidationError("map rows must have equal widths", path=["mapVariants", map_index, "rows", row_index])
        map_dimensions.append((len(rows), width))

    for start_index, position in enumerate(data["startingPositions"]):
        for field in ("y", "x"):
            if type(position[field]) is not int:
                raise ValidationError("expected an integer JSON value", path=["startingPositions", start_index, field])
        if map_dimensions and not all(position["y"] < height and position["x"] < width for height, width in map_dimensions):
            raise ValidationError("starting position must be within every map variant", path=["startingPositions", start_index])


def require_integer(value, path: list) -> None:
    # JSON Schema accepts 1.0 as an integer; info_set_integer rejects it.
    if type(value) is not int:
        raise ValidationError("expected an integer JSON value", path=path)


def find_companion_path(schema_path: Path, data_path: Path, name: str) -> Path:
    """隣接する候補定義を優先し、なければ同梱定義のパスを返す。"""
    companion_path = data_path.parent / f"{name}.jsonc"
    if not companion_path.exists():
        companion_path = schema_path.resolve().parent.parent / "lib/edit" / f"{name}.jsonc"
    return companion_path


def load_validated_companion(schema_path: Path, data_path: Path, registry: Registry,
                             name: str, label: str, semantic_validator: Callable[[dict], None],
                             *, selected_path: Path | None = None) -> dict:
    """隣接する候補定義を優先し、schemaと意味検証を終えた付随データを返す。"""
    companion_path = selected_path if selected_path is not None else find_companion_path(schema_path, data_path, name)
    companion = load_jsonc(companion_path)
    companion_schema_path = schema_path.parent / f"{name}.schema.json"
    companion_schema = load_jsonc(companion_schema_path)
    schema_uri = companion_schema_path.resolve().as_uri()
    absolute_schema = {**companion_schema, "$id": urljoin(schema_uri, companion_schema.get("$id", schema_uri))}
    try:
        validate(instance=companion, schema=absolute_schema, registry=registry)
        semantic_validator(companion)
    except ValidationError as e:
        raise ValueError(f"Invalid {label} definitions in {companion_path}: {e.message}; Location: {list(e.path)}") from e
    return companion


def load_dungeon_wilderness_dimensions(schema_path: Path, data_path: Path, registry: Registry) -> tuple[int, int]:
    """入口の検証には隣の候補荒野定義を優先し、なければ同梱定義を使う。"""
    wilderness = load_validated_companion(schema_path, data_path, registry, "WildernessDefinition", "wilderness",
                                          lambda companion: validate_wilderness_semantics(companion, schema_path))
    return wilderness["height"], wilderness["width"]


def validate_dungeon_semantics(data: dict, schema_path: Path, data_path: Path, registry: Registry) -> None:
    """DungeonReaderの整数型・ID順序・地形抽選の合計を検証する。"""
    dimensions = None
    if any(dungeon["id"] != 0 for dungeon in data["dungeons"]):
        dimensions = load_dungeon_wilderness_dimensions(schema_path, data_path, registry)
    previous_id = -1
    for index, dungeon in enumerate(data["dungeons"]):
        path = ["dungeons", index]
        require_integer(dungeon["id"], path + ["id"])
        if dungeon["id"] <= previous_id:
            raise ValidationError("dungeon IDs must be strictly increasing", path=path + ["id"])
        previous_id = dungeon["id"]
        fields = {"position": ("wild_y", "wild_x"),
                  "generation": ("minDepth", "maxDepth", "minPlayerLevel", "objGood", "objGreat"),
                  "monsters": ("minCount", "additionalSpawnProbability", "normalMonsterRate"),
                  "floor": ("tunnelRate",)}
        for group, names in fields.items():
            for name in names:
                require_integer(dungeon[group][name], path + [group, name])
        if dungeon["id"] != 0 and dimensions is not None:
            # ダンジョンを出ると荒野の現在地が入口になるため、DungeonReaderは荒野の端を拒否する。
            for field, extent in zip(("wild_y", "wild_x"), dimensions):
                if not 1 <= dungeon["position"][field] <= extent - 2:
                    raise ValidationError("dungeon entrance must be inside the wilderness map excluding the edges", path=path + ["position", field])
        if dungeon["generation"]["minDepth"] > dungeon["generation"]["maxDepth"]:
            raise ValidationError("minDepth must not exceed maxDepth", path=path + ["generation", "minDepth"])
        for group in ("floor", "wall"):
            total = 0
            for tile_index, tile in enumerate(dungeon[group]["tiles"]):
                weight_path = path + [group, "tiles", tile_index, "rate"]
                require_integer(tile["rate"], weight_path)
                total += tile["rate"]
                if total > 2147483647:
                    raise ValidationError("total weight exceeds the reader's integer range", path=weight_path)
            if total == 0:
                raise ValidationError("tiles require a positive total weight", path=path + [group, "tiles"])
        for stream_index, stream in enumerate(dungeon["wall"]["streams"]):
            for name in ("count", "chance", "priority"):
                require_integer(stream[name], path + ["wall", "streams", stream_index, name])
        for name, value in dungeon.get("final_floor", {}).items():
            require_integer(value, path + ["final_floor", name])


def load_cpp_map_keys(path: Path, table: str) -> set[str]:
    """指定したC++辞書のキーだけを読む。隣の辞書やコメントは含めない。"""
    source = path.read_text(encoding="utf-8")
    match = re.search(r'\b' + re.escape(table) + r'\s*=\s*\{(.*?)^\};', source, re.DOTALL | re.MULTILINE)
    if not match:
        raise ValueError(f"Definition table {table} not found in {path}")
    keys = set(re.findall(r'^\s*\{\s*"([^"\\]+)"\s*,', match[1], re.MULTILINE))
    if not keys:
        raise ValueError(f"No definition tokens found in {table}: {path}")
    return keys


def validate_terrain_semantics(data: dict, schema_path: Path) -> None:
    """地形の整数・表示・参照・状態数を確認する。重複定義はCIで拒否する。"""
    repository_root = schema_path.resolve().parent.parent
    flags = load_cpp_map_keys(repository_root / "src/info-reader/feature-info-tokens-table.cpp", "f_info_flags")
    colors = load_cpp_map_keys(repository_root / "src/term/gameterm.cpp", "color_list")
    ids = set()
    tags = set()
    for terrain_index, terrain in enumerate(data["terrains"]):
        path = ["terrains", terrain_index]
        require_integer(terrain["id"], path + ["id"])
        if terrain["id"] in ids:
            raise ValidationError("terrain IDs must be unique; duplicate IDs overwrite definitions", path=path + ["id"])
        ids.add(terrain["id"])
        if terrain["key"] in tags:
            raise ValidationError("terrain tags must be unique; references would be ambiguous", path=path + ["key"])
        tags.add(terrain["key"])
        if terrain["symbol"]["color"] not in colors:
            raise ValidationError("unknown terrain color token", path=path + ["symbol", "color"])
        for flag_index, flag in enumerate(terrain["flags"]):
            if flag and flag not in flags:
                raise ValidationError("unknown terrain flag token", path=path + ["flags", flag_index])
        if terrain.get("map_priority") is not None:
            require_integer(terrain["map_priority"], path + ["map_priority"])
        for field in ("trap", "door", "tunnel"):
            if terrain.get(field) is not None:
                require_integer(terrain[field]["power"], path + [field, "power"])
        conversion = terrain.get("convert")
        if conversion is not None and "stream_index" in conversion:
            require_integer(conversion["stream_index"], path + ["convert", "stream_index"])
        generation = terrain.get("generation")
        if generation is None:
            continue
        probability_sum = 0
        for change_index, change in enumerate(generation["changes"]):
            probability_path = path + ["generation", "changes", change_index, "probability"]
            require_integer(change["probability"], probability_path)
            probability_sum += change["probability"]
            if probability_sum > 100:
                raise ValidationError("generation probability sum must not exceed 100", path=probability_path)

    # 候補文書内の全タグを集めてから参照を確認し、前方参照を許可する。
    def require_tag(tag, reference_path):
        if tag not in tags:
            raise ValidationError("undefined terrain tag", path=reference_path)

    for terrain_index, terrain in enumerate(data["terrains"]):
        path = ["terrains", terrain_index]
        mimic = terrain.get("mimic")
        if mimic:
            require_tag(mimic, path + ["mimic"])
        for change_index, change in enumerate((terrain.get("generation") or {}).get("changes", [])):
            require_tag(change["terrain"], path + ["generation", "changes", change_index, "terrain"])
        state_count = 0
        # nlohmann::jsonのobjectと同じキー順で、9番目の状態を診断する。
        for action, value in sorted((terrain.get("interactions") or {}).items()):
            action_path = path + ["interactions", action]
            if action == "DROP_GOLD":
                # schemaは正のNdMを保証する。書式とintの範囲はアイテムのダイスと同じ規則で確かめる。
                require_reader_dice(value, action_path)
                continue
            if action == "DESTROYED":
                require_tag(value, action_path)
                continue
            if state_count >= 8:
                raise ValidationError("terrain interactions must not exceed eight states", path=action_path)
            if action not in flags:
                raise ValidationError("unknown terrain action token", path=action_path)
            require_tag(value, action_path)
            state_count += 1


def load_class_ids(repository_root: Path) -> dict[str, int]:
    """Read MagicReader's tokens and PlayerClassType values, not map order."""
    enum_path = repository_root / "src/player-info/class-types.h"
    enum_source = enum_path.read_text(encoding="utf-8")
    enum_match = re.search(r'enum class PlayerClassType\s*:[^{]+\{([^}]+)\}', enum_source)
    if not enum_match:
        raise ValueError(f"PlayerClassType definition not found in {enum_path}")
    entries = re.sub(r'/\*.*?\*/|//[^\n]*', '', enum_match[1], flags=re.DOTALL)
    enum_ids = {}
    next_id = 0
    for entry in entries.split(','):
        if not entry.strip():
            continue
        match = re.fullmatch(r'\s*([A-Z][A-Z0-9_]*)(?:\s*=\s*(-?\d+))?\s*', entry)
        if not match:
            raise ValueError(f"Unsupported PlayerClassType value in {enum_path}: {entry.strip()}")
        name, explicit_id = match.groups()
        if explicit_id is not None:
            next_id = int(explicit_id)
        enum_ids[name] = next_id
        next_id += 1
    reader_path = repository_root / "src/info-reader/magic-reader.cpp"
    tokens = re.findall(r'\{\s*"([A-Z][A-Z0-9_]*)"\s*,\s*PlayerClassType::([A-Z][A-Z0-9_]*)\s*\}',
                        reader_path.read_text(encoding="utf-8"))
    if not tokens or any(name not in enum_ids for _, name in tokens):
        raise ValueError(f"Invalid class token mapping in {reader_path}")
    return {token: enum_ids[name] for token, name in tokens}


def load_spell_tags(schema_path: Path, data_path: Path, registry: Registry) -> dict[str, set[str]]:
    """Use adjacent candidate definitions, or the schema repository's bundled data."""
    spells = load_validated_companion(schema_path, data_path, registry,
                                     "SpellDefinitions", "spell", validate_spell_semantics)
    tag_ids = {}
    for realm in spells["realms"]:
        realm_tag_ids = tag_ids.setdefault(realm["name"], {})
        for book in realm["books"]:
            for spell in book["spells"]:
                realm_tag_ids[spell["spell_tag"]] = spell["spell_id"]
    tags = {}
    for realm, realm_tag_ids in tag_ids.items():
        ids = set(realm_tag_ids.values())
        # Undefined slots also have empty tags, and get_spell_id returns the first match.
        # An empty tag is referenceable only when every lower slot has a definition.
        tags[realm] = {tag for tag, spell_id in realm_tag_ids.items()
                       if tag or all(lower_id in ids for lower_id in range(spell_id))}
    return tags


def validate_class_magic_semantics(data: dict, schema_path: Path, data_path: Path, registry: Registry) -> None:
    """Check MagicReader's strict integers, class order and realm spell references."""
    class_ids = load_class_ids(schema_path.resolve().parent.parent)
    spell_tags = load_spell_tags(schema_path, data_path, registry)
    previous_id = -1
    for class_index, record in enumerate(data["classes"]):
        path = ["classes", class_index]
        class_id = class_ids.get(record["name"])
        if class_id is None:
            raise ValidationError("unknown class token", path=path + ["name"])
        if class_id < previous_id:
            raise ValidationError("class IDs must be in nondecreasing order", path=path + ["name"])
        previous_id = class_id
        for field in ("first_spell_level", "armour_weight_limit"):
            require_integer(record[field], path + [field])
        for realm_index, realm in enumerate(record["realms"]):
            for spell_index, spell in enumerate(realm["spells_info"]):
                spell_path = path + ["realms", realm_index, "spells_info", spell_index]
                if spell["spell_tag"] not in spell_tags.get(realm["name"], set()):
                    raise ValidationError("unknown spell tag in this realm", path=spell_path + ["spell_tag"])
                for field in ("learn_level", "mana_cost", "difficulty", "first_cast_exp_rate"):
                    require_integer(spell[field], spell_path + [field])


def validate_class_skill_semantics(data: dict) -> None:
    """Check SkillReader's strict integers, sequential IDs and start limits."""
    for class_index, record in enumerate(data["classes"]):
        path = ["classes", class_index]
        require_integer(record["id"], path + ["id"])
        if record["id"] != class_index:
            raise ValidationError("class IDs must be sequential starting at 0", path=path + ["id"])
        for name, weapon in record["weapons"].items():
            weapon_path = path + ["weapons", name]
            for field in ("start_ranks", "max_ranks"):
                for rank_index, rank in enumerate(weapon[field]):
                    require_integer(rank, weapon_path + [field, rank_index])
            for rank_index, (start, maximum) in enumerate(zip(weapon["start_ranks"], weapon["max_ranks"])):
                if start > maximum:
                    raise ValidationError("start rank must not exceed maximum rank", path=weapon_path + ["start_ranks", rank_index])
        for name, skill in record["skills"].items():
            skill_path = path + ["skills", name]
            for field in ("start_exp", "max_exp"):
                require_integer(skill[field], skill_path + [field])
            if skill["start_exp"] > skill["max_exp"]:
                raise ValidationError("start experience must not exceed maximum experience", path=skill_path + ["start_exp"])


def validate_spell_semantics(data: dict) -> None:
    """Reject strict integer failures, overwrites and ambiguous tag lookups."""
    realm_ids = {}
    realm_tags = {}
    for realm_index, realm in enumerate(data["realms"]):
        ids = realm_ids.setdefault(realm["name"], set())
        tags = realm_tags.setdefault(realm["name"], set())
        for book_index, book in enumerate(realm["books"]):
            for spell_index, spell in enumerate(book["spells"]):
                path = ["realms", realm_index, "books", book_index, "spells", spell_index, "spell_id"]
                require_integer(spell["spell_id"], path)
                if spell["spell_id"] in ids:
                    raise ValidationError("spell IDs must be unique within each realm", path=path)
                ids.add(spell["spell_id"])
                tag_path = path[:-1] + ["spell_tag"]
                if spell["spell_tag"] in tags:
                    raise ValidationError("spell tags must be unique within each realm", path=tag_path)
                tags.add(spell["spell_tag"])


# 小さなID集合だけを保持し、一時候補の検証を繰り返しても保持件数を制限する。
_companion_id_cache = OrderedDict()
MAX_COMPANION_ID_CACHE_ENTRIES = 16


def companion_digest(path: Path) -> bytes:
    """時刻・サイズが同じ書換えも検出し、大きな参照定義は分割してハッシュする。"""
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(65536), b""):
            digest.update(block)
    return digest.digest()


def extract_monster_ids(data: dict) -> frozenset[int]:
    """schemaの整数判定を通る小数も、ID集合を作る前に拒否する。"""
    for index, monster in enumerate(data["monsters"]):
        require_integer(monster["id"], ["monsters", index, "id"])
    return frozenset(monster["id"] for monster in data["monsters"])


def load_companion_ids(schema_path: Path, data_path: Path, registry: Registry, name: str, label: str,
                       extract: Callable[[dict], frozenset]) -> frozenset:
    """同じ内容・schema・検証コンテキストの付随定義から作ったID集合を再利用する。"""
    companion_path = find_companion_path(schema_path, data_path, name).resolve()
    companion_schema_path = (schema_path.parent / f"{name}.schema.json").resolve()
    key = (companion_path, companion_schema_path)
    fingerprint = (companion_digest(companion_path), companion_digest(companion_schema_path))
    cached = _companion_id_cache.get(key)
    if cached is not None and cached[0] is registry and cached[1] == fingerprint:
        _companion_id_cache.move_to_end(key)
        return cached[2]

    # 検証と集合作りを1回で済ませるため、検証関数として渡した extract の結果を受け取る。
    extracted = []
    load_validated_companion(schema_path, data_path, registry, name, label, lambda data: extracted.append(extract(data)),
                             selected_path=companion_path)
    ids = extracted[0]
    # 読込中に更新された場合は、異なる内容のキーでキャッシュしない。
    if fingerprint == (companion_digest(companion_path), companion_digest(companion_schema_path)):
        _companion_id_cache[key] = (registry, fingerprint, ids)
        _companion_id_cache.move_to_end(key)
        if len(_companion_id_cache) > MAX_COMPANION_ID_CACHE_ENTRIES:
            _companion_id_cache.popitem(last=False)
    return ids


def load_defined_monster_ids(schema_path: Path, data_path: Path, registry: Registry) -> frozenset[int]:
    return load_companion_ids(schema_path, data_path, registry, "MonraceDefinitions", "monster", extract_monster_ids)


def validate_monster_message_semantics(data: dict, schema_path: Path, data_path: Path, registry: Registry) -> None:
    defined_monrace_ids = load_defined_monster_ids(schema_path, data_path, registry)
    for group_index, group in enumerate(data["groups"]):
        group_path = ["groups", group_index]
        if "id_list" in group:
            for id_index, monster_id in enumerate(group["id_list"]):
                require_integer(monster_id, group_path + ["id_list", id_index])
                # MessageReaderはモンスター種族の定義に無いIDを拒否する。
                if monster_id not in defined_monrace_ids:
                    raise ValidationError("unknown monster ID", path=group_path + ["id_list", id_index])
        # 実行時の到達可否によらず、全メッセージの入力を検証する。
        for message_index, message in enumerate(group["message"]):
            message_path = group_path + ["message", message_index]
            require_integer(message["chance"], message_path + ["chance"])
            languages = message["message"]
            if not languages:
                raise ValidationError("a message must contain ja or en", path=message_path + ["message"])

        # 読込を終えた言語の後続メッセージが黙って捨てられる定義を拒否する。
        missing_languages = set()
        for message_index, message in enumerate(group["message"]):
            languages = message["message"]
            for language in ("ja", "en"):
                if language not in languages:
                    missing_languages.add(language)
                elif language in missing_languages:
                    path = group_path + ["message", message_index, "message", language]
                    raise ValidationError(f"{language} messages after a missing locale would be discarded", path=path)


MAX_MONRACE_DICE_ROLL = 32767


def validate_monrace_semantics(data: dict, schema_path: Path, data_path: Path, registry: Registry) -> None:
    """Check RaceReader's strict integers, ID order, dice limits and references."""
    # 参照先として読むアーティファクト定義は、元のベースアイテムの実在までは確かめない。
    # アーティファクト定義そのものの検証で確かめるため、ここではIDを集めるのに必要な検証にとどめる。
    artifact_data = load_validated_companion(schema_path, data_path, registry,
                                             "ArtifactDefinitions", "artifact", validate_artifact_semantics)
    defined_artifact_ids = {artifact["id"] for artifact in artifact_data["artifacts"]}
    monraces = data["monsters"]
    defined_monrace_ids = {monrace["id"] for monrace in monraces if type(monrace["id"]) is int}

    def check_dice(value, path):
        # pattern の $ は末尾の改行の前にも一致するため、文字列全体で確かめる。
        if not re.fullmatch(r"[1-9][0-9]*d[1-9][0-9]*", value):
            raise ValidationError("dice must be written as XdY", path=path)
        # 最大の出目(個数×面数)は捕獲したモンスターのHPを保持するshortに収める。
        num, sides = (int(token) for token in value.split("d"))
        if num * sides > MAX_MONRACE_DICE_ROLL:
            raise ValidationError(f"dice maximum roll must not exceed {MAX_MONRACE_DICE_ROLL}", path=path)

    def check_monrace_reference(value, path):
        # ID 0 はプレイヤーを表し、モンスター種族として参照できない。
        if value == 0 or value not in defined_monrace_ids:
            raise ValidationError("unknown monster ID", path=path)

    previous_id = -1
    for index, monrace in enumerate(monraces):
        path = ["monsters", index]
        for field in ("id", "speed", "vision", "armor_class", "alertness", "level", "rarity", "exp",
                      "odds_correction_ratio", "start_hp_percentage"):
            if field in monrace:
                require_integer(monrace[field], path + [field])
        if monrace["id"] <= previous_id:
            raise ValidationError("monster IDs must be strictly increasing", path=path + ["id"])
        previous_id = monrace["id"]

        # RaceReaderは表示文字を1バイトちょうどに限る。
        if len(monrace["symbol"]["character"].encode("utf-8")) != 1:
            raise ValidationError("symbol character must be a single byte", path=path + ["symbol", "character"])
        check_dice(monrace["hit_point"], path + ["hit_point"])
        if "evolve" in monrace:
            for field in ("need_exp", "to"):
                require_integer(monrace["evolve"][field], path + ["evolve", field])
            check_monrace_reference(monrace["evolve"]["to"], path + ["evolve", "to"])
        for artifact_index, artifact in enumerate(monrace.get("artifacts", [])):
            artifact_path = path + ["artifacts", artifact_index]
            for field in ("drop_artifact_id", "drop_probability"):
                require_integer(artifact[field], artifact_path + [field])
            if artifact["drop_artifact_id"] not in defined_artifact_ids:
                raise ValidationError("unknown artifact ID", path=artifact_path + ["drop_artifact_id"])
        for escort_index, escort in enumerate(monrace.get("escorts", [])):
            escort_path = path + ["escorts", escort_index]
            require_integer(escort["escorts_id"], escort_path + ["escorts_id"])
            check_monrace_reference(escort["escorts_id"], escort_path + ["escorts_id"])
            check_dice(escort["escort_num"], escort_path + ["escort_num"])
        for blow_index, blow in enumerate(monrace.get("blows", [])):
            if "damage_dice" in blow:
                check_dice(blow["damage_dice"], path + ["blows", blow_index, "damage_dice"])
        if "skill" in monrace:
            # pattern の $ は末尾の改行の前にも一致するため、文字列全体で確かめる。
            if not re.fullmatch(r"1_IN_([1-9][0-9]?|100)", monrace["skill"]["probability"]):
                raise ValidationError("probability must be written as 1_IN_n", path=path + ["skill", "probability"])
            if "shoot" in monrace["skill"]:
                check_dice(monrace["skill"]["shoot"], path + ["skill", "shoot"])
        for summon_index, summon in enumerate(monrace.get("final_summon", [])):
            summon_path = path + ["final_summon", summon_index]
            for field in ("id", "probability", "min_num", "max_num", "radius"):
                require_integer(summon[field], summon_path + [field])
            check_monrace_reference(summon["id"], summon_path + ["id"])
            if summon["min_num"] > summon["max_num"]:
                raise ValidationError("min_num must not exceed max_num", path=summon_path + ["min_num"])
        for message_index, message in enumerate(monrace.get("message", [])):
            require_integer(message["chance"], path + ["message", message_index, "chance"])


def validate_one(pair: tuple[Path, Path, dict], registry: Registry | None = None) -> tuple[bool, str]:
    data_path, schema_path, schema = pair
    try:
        data = load_jsonc(data_path)
        if registry is None:
            loaded_schemas = load_all_schemas(schema_path.parent)[0] if schema_path.is_file() else {}
            registry = build_schema_registry(loaded_schemas)
        schema_uri = schema_path.resolve().as_uri()
        absolute_schema = schema if isinstance(schema, bool) else {**schema, "$id": urljoin(schema_uri, schema.get("$id", schema_uri))}
        validate(instance=data, schema=absolute_schema, registry=registry)
        semantic_validators = {
            "DungeonDefinitions.schema.json": lambda: validate_dungeon_semantics(data, schema_path, data_path, registry),
            "TerrainDefinitions.schema.json": lambda: validate_terrain_semantics(data, schema_path),
            "VaultDefinitions.schema.json": lambda: validate_vault_semantics(data),
            "EgoDefinitions.schema.json": lambda: validate_ego_semantics(data, schema_path),
            "BaseitemDefinitions.schema.json": lambda: validate_baseitem_semantics(data, schema_path),
            "ArtifactDefinitions.schema.json": lambda: validate_artifact_semantics(data, load_valid_baseitem_keys(schema_path, data_path, registry)),
            "WildernessDefinition.schema.json": lambda: validate_wilderness_semantics(data, schema_path),
            "TownPreferences.schema.json": lambda: validate_town_preferences_semantics(data, schema_path),
            "TownDefinitionList.schema.json": lambda: validate_town_definition_list_semantics(data, schema_path, data_path, registry),
            "TownMap.schema.json": lambda: validate_town_map_semantics(data, schema_path),
            "MonraceDefinitions.schema.json": lambda: validate_monrace_semantics(data, schema_path, data_path, registry),
            "ClassMagicDefinitions.schema.json": lambda: validate_class_magic_semantics(data, schema_path, data_path, registry),
            "ClassSkillDefinitions.schema.json": lambda: validate_class_skill_semantics(data),
            "SpellDefinitions.schema.json": lambda: validate_spell_semantics(data),
            "MonsterMessages.schema.json": lambda: validate_monster_message_semantics(data, schema_path, data_path, registry),
            "Quest.schema.json": lambda: validate_quest_file_semantics(data, schema_path, data_path, registry),
            "QuestPreferences.schema.json": lambda: validate_quest_preferences_semantics(data, schema_path, data_path, registry),
        }
        semantic_validator = semantic_validators.get(schema_path.name)
        if semantic_validator is not None:
            semantic_validator()
        return True, f"Succeeded: {data_path.name} <= {schema_path.name}"
    except ValidationError as e:
        msg = [f"Failed: {data_path.name}", f"Reason: {e.message}"]
        if e.path:
            msg.append(f"Location: {list(e.path)}")

        return False, "\n".join(msg)
    except SchemaError as e:
        return False, f"Schema Error in {schema_path.name}: {e.message}"
    except Unresolvable as e:
        return False, f"Schema Reference Error in {schema_path.name}: {e}"
    except (IOError, ValueError, RuntimeError, Json5Exception) as e:
        return False, f"Error: {data_path.name} - {e}"


def main():
    base = Path(__file__).parent.parent.parent
    schema_dir = base / "schema"
    edit_dir = base / "lib" / "edit"
    if not schema_dir.is_dir() or not edit_dir.is_dir():
        print(f"Error: Directory not found. schema: {schema_dir}, edit: {edit_dir}", file=sys.stderr)
        sys.exit(1)

    print(f"Hengband JSONC validation started! Loading schemas...")

    loaded_schemas, schema_map = load_all_schemas(schema_dir)
    registry = build_schema_registry(loaded_schemas)
    pairs = build_validation_pairs(edit_dir, schema_map, loaded_schemas)
    print(f"Validation target: {len(pairs)} files")

    success = 0
    for p in pairs:
        ok, message = validate_one(p, registry)
        print(message, file=sys.stdout if ok else sys.stderr)
        if ok:
            success += 1

    print()
    print(f"Finished! {success}/{len(pairs)} successful")
    sys.exit(0 if success == len(pairs) else 1)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"Error - {e}", file=sys.stderr)
        sys.exit(1)
