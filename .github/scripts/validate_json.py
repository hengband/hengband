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


def validate_one(pair: tuple[Path, Path, dict]) -> tuple[bool, str]:
    data_path, schema_path, schema = pair
    try:
        data = load_jsonc(data_path)
        validate(instance=data, schema=schema)
        if schema_path.name == "VaultDefinitions.schema.json":
            validate_vault_semantics(data)
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
