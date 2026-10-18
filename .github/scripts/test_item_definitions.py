"""ベースアイテム・アーティファクト定義の schema と意味検証が reader の受理範囲と一致することを確かめる。"""
import copy
from pathlib import Path
import unittest

from json_validation_test_helper import validate_document
from validate_json import build_schema_registry, load_all_schemas, load_jsonc, load_rod_type_value


class BaseitemDefinitionValidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[2]
        cls.loaded, cls.schema_map = load_all_schemas(root / "schema")
        cls.registry = build_schema_registry(cls.loaded)
        cls.document = load_jsonc(root / "lib/edit/BaseitemDefinitions.jsonc")
        baseitems = cls.document["baseitems"]
        cls.sample = next(item for item in baseitems if {"parameter_value", "base_dice", "allocations"} <= item.keys())
        rod_type_value = load_rod_type_value(root)
        cls.rod = next(item for item in baseitems if item["itemkind"]["type_value"] == rod_type_value)

    def validate(self, *baseitems):
        schema_path = self.schema_map["BaseitemDefinitions"]
        document = {"version": self.document["version"], "baseitems": list(baseitems)}
        return validate_document(document, schema_path, self.loaded[schema_path], self.registry)

    def check(self, record, accepted, location=None, reason=None):
        ok, message = self.validate(record)
        self.assertEqual(ok, accepted, message)
        if location is not None:
            self.assertIn(f"Location: {location}", message)
        if reason is not None:
            self.assertIn(reason, message)

    def changed(self, base, path, value):
        record = copy.deepcopy(base)
        target = record
        for part in path[:-1]:
            target = target[part]
        target[path[-1]] = value
        return record

    def test_bundled_definitions(self):
        ok, message = self.validate(*self.document["baseitems"])
        self.assertTrue(ok, message)

    def test_integer_representation_matches_reader(self):
        paths = [[field] for field in ("id", "parameter_value", "level", "weight", "cost")]
        paths += [["itemkind", "type_value"], ["itemkind", "subtype_value"], ["allocations", 0, "depth"], ["allocations", 0, "rarity"]]
        for path in paths:
            with self.subTest(path=path):
                value = copy.deepcopy(self.sample)
                for part in path[:-1]:
                    value = value[part]
                record = self.changed(self.sample, path, float(value[path[-1]]))
                self.check(record, False, ["baseitems", 0] + path, "integer JSON value")
        for field in ("base_ac", "hit_bonus", "damage_bonus", "ac_bonus"):
            with self.subTest(field=field):
                self.check(self.changed(self.sample, [field], 1.0), False, ["baseitems", 0, field], "integer JSON value")

    def test_ids_must_be_strictly_increasing(self):
        for second_id in (self.sample["id"], self.sample["id"] - 1):
            with self.subTest(second_id=second_id):
                ok, message = self.validate(self.sample, {**self.sample, "id": second_id})
                self.assertFalse(ok, message)
                self.assertIn("Location: ['baseitems', 1, 'id']", message)
        ok, message = self.validate(self.sample, {**self.sample, "id": self.sample["id"] + 2})
        self.assertTrue(ok, message)

    def test_symbol_and_level_are_required(self):
        for field in ("symbol", "level"):
            with self.subTest(field=field):
                record = copy.deepcopy(self.sample)
                del record[field]
                self.check(record, False, reason=f"'{field}' is a required property")

    def test_symbol_matches_reader(self):
        for character, accepted in (("\\", True), ("~", True), ("", False), ("ab", False), ("é", False)):
            with self.subTest(character=character):
                record = self.changed(self.sample, ["symbol", "character"], character)
                self.check(record, accepted, None if accepted else ["baseitems", 0, "symbol", "character"])
        for color, accepted in (("Light Brown", True), ("Black", True), ("Brown ", False), ("Pink", False)):
            with self.subTest(color=color):
                record = self.changed(self.sample, ["symbol", "color"], color)
                self.check(record, accepted, None if accepted else ["baseitems", 0, "symbol", "color"])

    def test_rod_parameter_value_must_be_positive(self):
        self.check(self.changed(self.rod, ["parameter_value"], 1), True)
        self.check(self.changed(self.rod, ["parameter_value"], 0), False, ["baseitems", 0, "parameter_value"], "must be positive")
        record = copy.deepcopy(self.rod)
        del record["parameter_value"]
        self.check(record, False, ["baseitems", 0, "parameter_value"], "must be positive")
        self.check(self.changed(self.sample, ["parameter_value"], 0), True)

    def test_dice_matches_reader(self):
        # from_chars は先頭の0をいくつでも読み飛ばす。
        for dice in ("1d4", "0d0", "-1d-2", "007d08", "2147483647d-2147483648", "0" * 5000 + "1d4"):
            with self.subTest(dice=dice[-8:]):
                self.check(self.changed(self.sample, ["base_dice"], dice), True)
        for dice in ("d", "1d", "d4", "1d4\n", "1d2d3", "+1d4", "1D4", " 1d4", "xd5y", "2147483648d1", "1d-2147483649",
                     "1" + "0" * 5000 + "d4"):
            with self.subTest(dice=dice[:16]):
                self.check(self.changed(self.sample, ["base_dice"], dice), False, ["baseitems", 0, "base_dice"])

    def test_allocations_fit_reader_tables(self):
        allocation = {"depth": 1, "rarity": 1}
        self.check(self.changed(self.sample, ["allocations"], [allocation] * 4), True)
        self.check(self.changed(self.sample, ["allocations"], [allocation] * 5), False, ["baseitems", 0, "allocations"])
        self.check(self.changed(self.sample, ["allocations"], [1]), False, ["baseitems", 0, "allocations", 0])


class ArtifactDefinitionValidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[2]
        cls.loaded, cls.schema_map = load_all_schemas(root / "schema")
        cls.registry = build_schema_registry(cls.loaded)
        cls.document = load_jsonc(root / "lib/edit/ArtifactDefinitions.jsonc")
        cls.sample = next(artifact for artifact in cls.document["artifacts"]
                          if {"parameter_value", "base_ac", "base_dice", "hit_bonus", "damage_bonus", "ac_bonus"} <= artifact.keys())
        cls.monraces = load_jsonc(root / "lib/edit/MonraceDefinitions.jsonc")

    def validate(self, *artifacts):
        schema_path = self.schema_map["ArtifactDefinitions"]
        document = {"version": self.document["version"], "artifacts": list(artifacts)}
        return validate_document(document, schema_path, self.loaded[schema_path], self.registry)

    def check(self, record, accepted, location=None, reason=None):
        ok, message = self.validate(record)
        self.assertEqual(ok, accepted, message)
        if location is not None:
            self.assertIn(f"Location: {location}", message)
        if reason is not None:
            self.assertIn(reason, message)

    def test_bundled_definitions(self):
        ok, message = self.validate(*self.document["artifacts"])
        self.assertTrue(ok, message)

    def test_integer_representation_matches_reader(self):
        for field in ("id", "parameter_value", "level", "rarity", "weight", "cost", "base_ac", "hit_bonus", "damage_bonus", "ac_bonus"):
            with self.subTest(field=field):
                self.check({**self.sample, field: float(self.sample[field])}, False, ["artifacts", 0, field], "integer JSON value")
        for field in ("type_value", "subtype_value"):
            with self.subTest(field=field):
                base_item = {**self.sample["base_item"], field: float(self.sample["base_item"][field])}
                self.check({**self.sample, "base_item": base_item}, False, ["artifacts", 0, "base_item", field], "integer JSON value")

    def test_ids_must_be_strictly_increasing(self):
        for second_id in (self.sample["id"], self.sample["id"] - 1):
            with self.subTest(second_id=second_id):
                ok, message = self.validate(self.sample, {**self.sample, "id": second_id})
                self.assertFalse(ok, message)
                self.assertIn("Location: ['artifacts', 1, 'id']", message)
        ok, message = self.validate(self.sample, {**self.sample, "id": self.sample["id"] + 1})
        self.assertTrue(ok, message)

    def test_dice_matches_reader(self):
        for dice in ("1d4", "0d0", "-1d-2", "2147483647d1", "0" * 5000 + "1d4"):
            with self.subTest(dice=dice[-8:]):
                self.check({**self.sample, "base_dice": dice}, True)
        for dice in ("d", "1d", "1d4\n", "1d2d3", "+1d4", "2147483648d1"):
            with self.subTest(dice=dice):
                self.check({**self.sample, "base_dice": dice}, False, ["artifacts", 0, "base_dice"])

    def test_monrace_validation_checks_artifact_companion(self):
        # モンスター定義の検証は、参照先のアーティファクト定義にも同じ意味検証をかける。
        schema_path = self.schema_map["MonraceDefinitions"]
        companion = {"version": self.document["version"], "artifacts": [{**self.sample, "level": float(self.sample["level"])}]}
        ok, message = validate_document(self.monraces, schema_path, self.loaded[schema_path], self.registry,
                                        companions={"ArtifactDefinitions.jsonc": companion})
        self.assertFalse(ok, message)
        self.assertIn("Invalid artifact definitions", message)
        self.assertIn("integer JSON value", message)


if __name__ == "__main__":
    unittest.main()
