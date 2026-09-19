"""Regression tests for the actual JSONC validation entry point used in CI."""
import copy
import json
from pathlib import Path
import tempfile
import unittest

from validate_json import load_jsonc, validate_one


class VaultValidationTest(unittest.TestCase):
    def setUp(self):
        self.schema_path = Path(__file__).resolve().parents[2] / "schema/VaultDefinitions.schema.json"
        self.schema = load_jsonc(self.schema_path)
        self.record = {
            "id": 0, "name": "Test", "type": 7, "rating": 5,
            "height": 1, "width": 3, "layout": [" % "],
        }

    def validate(self, data):
        """Use a real temporary JSON file, including schema and semantic checks."""
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "VaultDefinitions.jsonc"
            target.write_text(json.dumps(data), encoding="utf-8")
            return validate_one((target, self.schema_path, self.schema))

    def test_valid_sparse_ids_and_spaces(self):
        second = {**self.record, "id": 3}
        ok, message = self.validate({"vaults": [self.record, second]})
        self.assertTrue(ok, message)

    def test_invalid_roots(self):
        for root in (None, [], {}, {"vaults": None}, {"vaults": {}}, {"vaults": []}):
            with self.subTest(root=root):
                self.assertFalse(self.validate(root)[0])

    def test_duplicate_and_descending_ids(self):
        for ids in ((0, 0), (2, 1)):
            with self.subTest(ids=ids):
                records = [{**self.record, "id": index} for index in ids]
                ok, message = self.validate({"vaults": records})
                self.assertFalse(ok)
                self.assertIn("increasing order", message)
                self.assertIn("['vaults', 1, 'id']", message)

    def test_layout_dimensions(self):
        for layout, reason in (([" % ", " % "], "row count"), (["%"], "row byte length"), (["%%%%"], "row byte length")):
            with self.subTest(layout=layout):
                record = {**self.record, "layout": layout}
                ok, message = self.validate({"vaults": [record]})
                self.assertFalse(ok)
                self.assertIn(reason, message)
                self.assertIn("['vaults', 0, 'layout'", message)

    def test_non_ascii_and_control_characters(self):
        for row in ("%\n", "\u00e9% ", "%\t ", "%\x00 "):
            with self.subTest(row=row):
                record = {**self.record, "layout": [row]}
                self.assertFalse(self.validate({"vaults": [record]})[0])

    def test_integer_representation_matches_reader(self):
        for field in ("id", "type", "rating", "height", "width"):
            with self.subTest(field=field):
                record = copy.deepcopy(self.record)
                record[field] = float(record[field])
                ok, message = self.validate({"vaults": [record]})
                self.assertFalse(ok)
                self.assertIn("integer JSON value", message)

    def test_other_schemas_do_not_require_vault_fields(self):
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "Other.jsonc"
            target.write_text('{"items": []}', encoding="utf-8")
            schema = {"type": "object", "properties": {"items": {"type": "array"}}}
            ok, message = validate_one((target, Path("Other.schema.json"), schema))
            self.assertTrue(ok, message)


class EgoValidationTest(unittest.TestCase):
    def setUp(self):
        self.schema_path = Path(__file__).resolve().parents[2] / "schema/EgoDefinitions.schema.json"
        self.schema = load_jsonc(self.schema_path)
        self.record = {
            "id": 4,
            "name": {"ja": "試験の", "en": "of Testing"},
            "slot": 31,
            "rating": 10,
            "level": 0,
            "rarity": 20,
            "cost": 1000,
            "maximum_bonuses": {"to_hit": 1, "to_damage": 2, "to_ac": 3, "pval": 4},
            "flags": ["STR"],
            "extra_flags": [{"numerator": 1, "denominator": 3, "flags": ["RES_FIRE"]}],
        }

    def validate(self, data):
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "EgoDefinitions.jsonc"
            target.write_text(json.dumps(data), encoding="utf-8")
            return validate_one((target, self.schema_path, self.schema))

    def test_valid_sparse_ids(self):
        second = {**self.record, "id": 8}
        ok, message = self.validate({"version": 1, "egos": [self.record, second]})
        self.assertTrue(ok, message)

    def test_duplicate_ids(self):
        records = [self.record, {**self.record}]
        ok, message = self.validate({"version": 1, "egos": records})
        self.assertFalse(ok)
        self.assertIn("unique", message)

    def test_descending_ids_are_supported(self):
        records = [{**self.record, "id": ego_id} for ego_id in (8, 4)]
        ok, message = self.validate({"version": 1, "egos": records})
        self.assertTrue(ok, message)

    def test_integer_representation_matches_reader(self):
        for field in ("id", "slot", "rating", "level", "rarity", "cost"):
            with self.subTest(field=field):
                record = copy.deepcopy(self.record)
                record[field] = float(record[field])
                ok, message = self.validate({"version": 1, "egos": [record]})
                self.assertFalse(ok)
                self.assertIn("integer JSON value", message)

    def test_rarity_matches_byte_storage(self):
        record = {**self.record, "rarity": 255}
        ok, message = self.validate({"version": 1, "egos": [record]})
        self.assertTrue(ok, message)
        record["rarity"] = 256
        ok, message = self.validate({"version": 1, "egos": [record]})
        self.assertFalse(ok)

    def test_level_matches_depth_storage(self):
        record = {**self.record, "level": 32768}
        ok, message = self.validate({"version": 1, "egos": [record]})
        self.assertTrue(ok, message)
        record["level"] = 2147483648
        ok, message = self.validate({"version": 1, "egos": [record]})
        self.assertFalse(ok)

    def test_known_runtime_tokens_are_accepted_by_ci(self):
        record = copy.deepcopy(self.record)
        record["activation"] = "SUNLIGHT"
        record["flags"] = ["RES_FIRE", "CURSED"]
        record["extra_flags"][0]["flags"] = ["RES_COLD", "HEAVY_CURSE"]
        ok, message = self.validate({"version": 1, "egos": [record]})
        self.assertTrue(ok, message)

    def test_duplicate_flags_are_rejected_by_ci(self):
        for path in (("flags",), ("extra_flags", 0, "flags")):
            with self.subTest(path=path):
                record = copy.deepcopy(self.record)
                target = record
                for part in path[:-1]:
                    target = target[part]
                target[path[-1]] = ["RES_FIRE", "RES_FIRE"]
                ok, message = self.validate({"version": 1, "egos": [record]})
                self.assertFalse(ok)
                self.assertIn("non-unique", message)

    def test_unknown_runtime_tokens_are_rejected_by_ci(self):
        for path, value in (
            (("activation",), "UNKNOWN"),
            (("flags", 0), "UNKNOWN"),
            (("extra_flags", 0, "flags", 0), "UNKNOWN"),
        ):
            with self.subTest(path=path):
                record = copy.deepcopy(self.record)
                target = record
                for part in path[:-1]:
                    target = target[part]
                target[path[-1]] = value
                ok, message = self.validate({"version": 1, "egos": [record]})
                self.assertFalse(ok)
                self.assertIn("unknown", message)

    def test_nested_integer_representation_matches_reader(self):
        for path in (
            ("base_bonuses", "to_hit"),
            ("maximum_bonuses", "pval"),
            ("extra_flags", 0, "numerator"),
            ("extra_flags", 0, "denominator"),
        ):
            with self.subTest(path=path):
                record = copy.deepcopy(self.record)
                record.setdefault("base_bonuses", {"to_hit": 1, "to_damage": 2, "to_ac": 3})
                target = record
                for part in path[:-1]:
                    target = target[part]
                target[path[-1]] = float(target[path[-1]])
                ok, message = self.validate({"version": 1, "egos": [record]})
                self.assertFalse(ok)


class WildernessValidationTest(unittest.TestCase):
    def setUp(self):
        self.schema_path = Path(__file__).resolve().parents[2] / "schema/WildernessDefinition.schema.json"
        self.schema = load_jsonc(self.schema_path)
        self.data = {
            "version": 1,
            "width": 3,
            "height": 3,
            "towns": [{"id": 1, "name": {"ja": "街", "en": "Town"}}],
            "maps": {
                "normal": {
                    "letters": [
                        {"symbol": "#", "terrain": 0},
                        {"symbol": "1", "terrain": 1, "level": {"ja": 30, "en": 20}, "town": 1},
                    ],
                    "layout": ["###", "#1#", "###"],
                    "starting_position": {"x": 1, "y": 1},
                },
                "compact": {
                    "letters": [{"symbol": "#", "terrain": 0}],
                    "layout": ["###", "###", "###"],
                    "starting_position": {"x": 1, "y": 1},
                },
            },
        }

    def validate(self, data):
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "WildernessDefinition.jsonc"
            target.write_text(json.dumps(data), encoding="utf-8")
            return validate_one((target, self.schema_path, self.schema))

    def test_valid_localized_level_and_compact_map(self):
        ok, message = self.validate(self.data)
        self.assertTrue(ok, message)

    def test_duplicate_town_ids_and_symbols(self):
        duplicate_town = copy.deepcopy(self.data)
        duplicate_town["towns"].append(copy.deepcopy(duplicate_town["towns"][0]))
        self.assertFalse(self.validate(duplicate_town)[0])

        duplicate_symbol = copy.deepcopy(self.data)
        duplicate_symbol["maps"]["normal"]["letters"].append({"symbol": "#", "terrain": 0})
        self.assertFalse(self.validate(duplicate_symbol)[0])

    def test_normal_dimensions_and_defined_symbols(self):
        short_row = copy.deepcopy(self.data)
        short_row["maps"]["normal"]["layout"][1] = "##"
        self.assertFalse(self.validate(short_row)[0])

        undefined_symbol = copy.deepcopy(self.data)
        undefined_symbol["maps"]["normal"]["layout"][1] = "#.#"
        self.assertFalse(self.validate(undefined_symbol)[0])

    def test_starting_position_is_inside_layout(self):
        outside = copy.deepcopy(self.data)
        outside["maps"]["compact"]["starting_position"] = {"x": 3, "y": 1}
        self.assertFalse(self.validate(outside)[0])

    def test_integer_representation_matches_reader(self):
        for field in ("width", "height"):
            with self.subTest(field=field):
                data = copy.deepcopy(self.data)
                data[field] = float(data[field])
                self.assertFalse(self.validate(data)[0])


if __name__ == "__main__":
    unittest.main()
