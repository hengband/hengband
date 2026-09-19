import re
import sys
from pathlib import Path
import pyjson5
from pyjson5 import Json5Exception
from jsonschema import validate, ValidationError, SchemaError
from jsonschema.validators import validator_for


def load_jsonc(file_path: Path) -> dict:
    with file_path.open(encoding='utf-8-sig') as f:
        return pyjson5.load(f)


def load_all_schemas(schema_dir: Path) -> tuple[dict[Path, dict], dict[str, Path]]:
    schema_map: dict[str, Path] = {}
    loaded: dict[Path, dict] = {}
    errors: list[str] = []
    for s in schema_dir.glob("*.schema.json"):
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
            # JSON Schema accepts 1.0 as an integer; the C++ reader does not.
            if type(vault[field]) is not int:
                raise ValidationError("expected an integer JSON value", path=path + [field])
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
    """Check unique IDs and strict integer types expected by EgoReader."""
    repository_root = schema_path.resolve().parent.parent
    valid_flags = load_cpp_tokens(repository_root / "src/info-reader/baseitem-tokens-table.cpp")
    valid_activations = load_cpp_tokens(repository_root / "src/object-enchant/activation-info-table.cpp")
    ids = set()
    integer_fields = ("id", "slot", "rating", "level", "rarity", "cost")
    for index, ego in enumerate(data["egos"]):
        path = ["egos", index]
        for field in integer_fields:
            if type(ego[field]) is not int:
                raise ValidationError("expected an integer JSON value", path=path + [field])
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
                if type(value) is not int:
                    raise ValidationError("expected an integer JSON value", path=path + [group, field])
        for extra_index, extra in enumerate(ego.get("extra_flags", [])):
            for flag_index, flag in enumerate(extra["flags"]):
                if flag not in valid_flags:
                    raise ValidationError(
                        "unknown ego flag token",
                        path=path + ["extra_flags", extra_index, "flags", flag_index],
                    )
            for field in ("numerator", "denominator"):
                if type(extra[field]) is not int:
                    raise ValidationError(
                        "expected an integer JSON value",
                        path=path + ["extra_flags", extra_index, field],
                    )


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


def validate_one(pair: tuple[Path, Path, dict]) -> tuple[bool, str]:
    data_path, schema_path, schema = pair
    try:
        data = load_jsonc(data_path)
        validate(instance=data, schema=schema)
        if schema_path.name == "VaultDefinitions.schema.json":
            validate_vault_semantics(data)
        elif schema_path.name == "EgoDefinitions.schema.json":
            validate_ego_semantics(data, schema_path)
        elif schema_path.name == "WildernessDefinition.schema.json":
            validate_wilderness_semantics(data)
        return True, f"Succeeded: {data_path.name} <= {schema_path.name}"
    except ValidationError as e:
        msg = [f"Failed: {data_path.name}", f"Reason: {e.message}"]
        if e.path:
            msg.append(f"Location: {list(e.path)}")

        return False, "\n".join(msg)
    except SchemaError as e:
        return False, f"Schema Error in {schema_path.name}: {e.message}"
    except (IOError, ValueError, Json5Exception) as e:
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
    pairs = build_validation_pairs(edit_dir, schema_map, loaded_schemas)
    print(f"Validation target: {len(pairs)} files")

    success = 0
    for p in pairs:
        ok, message = validate_one(p)
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
