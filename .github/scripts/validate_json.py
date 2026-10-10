import re
import sys
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


def validate_wilderness_semantics(data: dict) -> None:
    """Check cross-field constraints used by WildernessReader."""
    for field in ("version", "width", "height"):
        if type(data[field]) is not int:
            raise ValidationError("expected an integer JSON value", path=[field])

    town_ids = set()
    for index, town in enumerate(data["towns"]):
        if type(town["id"]) is not int:
            raise ValidationError("expected an integer JSON value", path=["towns", index, "id"])
        if town["id"] in town_ids:
            raise ValidationError("town IDs must be unique", path=["towns", index, "id"])
        town_ids.add(town["id"])

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
        for row_index, row in enumerate(layout):
            if len(row) > width or (map_name == "normal" and len(row) != width):
                raise ValidationError("normal row width must equal width" if map_name == "normal" else "row width must not exceed width", path=map_path + ["layout", row_index])
            for column, symbol in enumerate(row):
                if symbol not in symbols:
                    raise ValidationError("layout contains an undefined symbol", path=map_path + ["layout", row_index, column])

        position = wilderness_map["starting_position"]
        for field in ("x", "y"):
            if type(position[field]) is not int:
                raise ValidationError("expected an integer JSON value", path=map_path + ["starting_position", field])
        if position["y"] >= len(layout) or position["x"] >= len(layout[position["y"]]):
            raise ValidationError("starting position must be within the map layout", path=map_path + ["starting_position"])


def validate_town_preferences_semantics(data: dict, schema_path: Path) -> None:
    """Check town legend values against the terrain definitions and reader types."""
    repository_root = schema_path.resolve().parent.parent
    terrain_data = load_jsonc(repository_root / "lib/edit/TerrainDefinitions.jsonc")
    valid_terrain_tags = {terrain["key"] for terrain in terrain_data["terrains"]}

    if type(data["version"]) is not int:
        raise ValidationError("expected an integer JSON value", path=["version"])
    for symbol, cell in data["legend"].items():
        terrain_tag = cell["terrain"]
        if terrain_tag != "*" and terrain_tag not in valid_terrain_tags:
            raise ValidationError("unknown terrain tag", path=["legend", symbol, "terrain"])
        if "special" in cell and type(cell["special"]) is not int:
            raise ValidationError("expected an integer JSON value", path=["legend", symbol, "special"])


def validate_town_definition_list_semantics(data: dict, schema_path: Path) -> None:
    """Check town map references against the distributed map files."""
    if type(data["version"]) is not int:
        raise ValidationError("expected an integer JSON value", path=["version"])

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
                if not -2147483648 <= int(fields[field_index]) <= 2147483647:
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
                if not -2147483648 <= int(value) <= 2147483647:
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


def validate_terrain_semantics(data: dict) -> None:
    """TerrainReaderの整数型と地形ごとの生成確率合計を確認する。"""
    for terrain_index, terrain in enumerate(data["terrains"]):
        path = ["terrains", terrain_index]
        require_integer(terrain["id"], path + ["id"])
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
    repository_root = schema_path.resolve().parent.parent
    spell_path = data_path.parent / "SpellDefinitions.jsonc"
    if not spell_path.exists():
        spell_path = repository_root / "lib/edit/SpellDefinitions.jsonc"
    spells = load_jsonc(spell_path)
    spell_schema_path = schema_path.parent / "SpellDefinitions.schema.json"
    spell_schema = load_jsonc(spell_schema_path)
    schema_uri = spell_schema_path.resolve().as_uri()
    absolute_schema = {**spell_schema, "$id": urljoin(schema_uri, spell_schema.get("$id", schema_uri))}
    try:
        validate(instance=spells, schema=absolute_schema, registry=registry)
        validate_spell_semantics(spells)
    except ValidationError as e:
        raise ValueError(f"Invalid spell definitions in {spell_path}: {e.message}; Location: {list(e.path)}") from e
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


def validate_monster_message_semantics(data: dict) -> None:
    for group_index, group in enumerate(data["groups"]):
        group_path = ["groups", group_index]
        if "id_list" in group:
            for id_index, monster_id in enumerate(group["id_list"]):
                require_integer(monster_id, group_path + ["id_list", id_index])
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
            "TerrainDefinitions.schema.json": lambda: validate_terrain_semantics(data),
            "VaultDefinitions.schema.json": lambda: validate_vault_semantics(data),
            "EgoDefinitions.schema.json": lambda: validate_ego_semantics(data, schema_path),
            "WildernessDefinition.schema.json": lambda: validate_wilderness_semantics(data),
            "TownPreferences.schema.json": lambda: validate_town_preferences_semantics(data, schema_path),
            "TownDefinitionList.schema.json": lambda: validate_town_definition_list_semantics(data, schema_path),
            "TownMap.schema.json": lambda: validate_town_map_semantics(data, schema_path),
            "ClassMagicDefinitions.schema.json": lambda: validate_class_magic_semantics(data, schema_path, data_path, registry),
            "ClassSkillDefinitions.schema.json": lambda: validate_class_skill_semantics(data),
            "SpellDefinitions.schema.json": lambda: validate_spell_semantics(data),
            "MonsterMessages.schema.json": lambda: validate_monster_message_semantics(data),
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
