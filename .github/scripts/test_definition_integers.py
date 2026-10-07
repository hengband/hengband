"""Reader integer and cross-field rules through the JSONC CI entry point."""
import copy
import json
from pathlib import Path
import tempfile
import unittest

from validate_json import build_schema_registry, load_all_schemas, load_jsonc, validate_one


class DefinitionIntegerValidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[2]
        cls.loaded, cls.schema_map = load_all_schemas(cls.root / "schema")
        cls.registry = build_schema_registry(cls.loaded)
        cls.documents = {
            name: load_jsonc(cls.root / "lib/edit" / f"{name}.jsonc")
            for name in ("ClassMagicDefinitions", "ClassSkillDefinitions", "SpellDefinitions")
        }
        magic = copy.deepcopy(cls.documents["ClassMagicDefinitions"])
        magic["classes"] = magic["classes"][1:2]
        magic["classes"][0]["realms"] = magic["classes"][0]["realms"][:1]
        magic["classes"][0]["realms"][0]["spells_info"] = magic["classes"][0]["realms"][0]["spells_info"][:1]
        spell = copy.deepcopy(cls.documents["SpellDefinitions"])
        spell["realms"] = spell["realms"][:1]
        spell["realms"][0]["books"] = spell["realms"][0]["books"][:1]
        spell["realms"][0]["books"][0]["spells"] = spell["realms"][0]["books"][0]["spells"][:1]
        cls.samples = {
            "ClassMagicDefinitions": magic,
            "ClassSkillDefinitions": cls.documents["ClassSkillDefinitions"],
            "SpellDefinitions": spell,
        }

    def validate(self, name, document):
        schema_path = self.schema_map[name]
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / f"{name}.jsonc"
            target.write_text(json.dumps(document), encoding="utf-8")
            return validate_one((target, schema_path, self.loaded[schema_path]), self.registry)

    def check_value(self, name, path, value, accepted, reason=None):
        document = copy.deepcopy(self.samples[name])
        target = document
        for part in path[:-1]:
            target = target[part]
        target[path[-1]] = value
        ok, message = self.validate(name, document)
        self.assertEqual(ok, accepted, message)
        if not accepted:
            self.assertIn(f"Location: {path}", message)
        if reason:
            self.assertIn(reason, message)

    def test_bundled_definitions(self):
        for name, document in self.documents.items():
            with self.subTest(name=name):
                ok, message = self.validate(name, document)
                self.assertTrue(ok, message)

    def test_magic_integer_fields(self):
        class_path = ["classes", 0]
        spell_path = class_path + ["realms", 0, "spells_info", 0]
        paths = [(class_path + [field]) for field in ("first_spell_level", "armour_weight_limit")]
        paths += [(spell_path + [field]) for field in ("learn_level", "mana_cost", "difficulty", "first_cast_exp_rate")]
        for path in paths:
            with self.subTest(path=path):
                self.check_value("ClassMagicDefinitions", path, 1.0, False, "integer JSON value")

    def test_skill_integer_fields(self):
        paths = [["classes", 0, "id"]]
        for weapon in ("BOW", "DIGGING", "HAFTED", "POLEARM", "SWORD"):
            for field in ("start_ranks", "max_ranks"):
                paths.append(["classes", 0, "weapons", weapon, field, 63])
        for skill in ("MARTIAL_ARTS", "TWO_WEAPON", "RIDING", "SHIELD"):
            for field in ("start_exp", "max_exp"):
                paths.append(["classes", 0, "skills", skill, field])
        for path in paths:
            with self.subTest(path=path):
                self.check_value("ClassSkillDefinitions", path, 0.0, False, "integer JSON value")

    def test_spell_integer_id(self):
        self.check_value("SpellDefinitions", ["realms", 0, "books", 0, "spells", 0, "spell_id"], 1.0, False, "integer JSON value")

    def test_other_noninteger_values_still_fail(self):
        fields = (
            ("ClassMagicDefinitions", ["classes", 0, "first_spell_level"]),
            ("ClassSkillDefinitions", ["classes", 0, "id"]),
            ("SpellDefinitions", ["realms", 0, "books", 0, "spells", 0, "spell_id"]),
        )
        for name, path in fields:
            for value in (True, "1", None, 1.5):
                with self.subTest(name=name, value=value):
                    self.check_value(name, path, value, False)

    def test_skill_class_order(self):
        for index, value in ((0, 1), (1, 0), (1, 2)):
            with self.subTest(index=index, value=value):
                self.check_value("ClassSkillDefinitions", ["classes", index, "id"], value, False, "sequential starting at 0")

    def test_skill_start_rank_exceeds_maximum(self):
        document = copy.deepcopy(self.samples["ClassSkillDefinitions"])
        weapon = document["classes"][0]["weapons"]["SWORD"]
        weapon["start_ranks"][63] = 4
        weapon["max_ranks"][63] = 3
        ok, message = self.validate("ClassSkillDefinitions", document)
        self.assertFalse(ok)
        self.assertIn("start rank must not exceed maximum rank", message)
        self.assertIn("Location: ['classes', 0, 'weapons', 'SWORD', 'start_ranks', 63]", message)

    def test_skill_start_experience_exceeds_maximum(self):
        document = copy.deepcopy(self.samples["ClassSkillDefinitions"])
        document["classes"][0]["skills"]["SHIELD"] = {"start_exp": 500, "max_exp": 499}
        ok, message = self.validate("ClassSkillDefinitions", document)
        self.assertFalse(ok)
        self.assertIn("start experience must not exceed maximum experience", message)
        self.assertIn("Location: ['classes', 0, 'skills', 'SHIELD', 'start_exp']", message)

    def test_skill_equal_limits(self):
        document = copy.deepcopy(self.samples["ClassSkillDefinitions"])
        weapon = document["classes"][0]["weapons"]["SWORD"]
        weapon["start_ranks"] = [4] * 64
        weapon["max_ranks"] = [4] * 64
        document["classes"][0]["skills"]["SHIELD"] = {"start_exp": 8000, "max_exp": 8000}
        ok, message = self.validate("ClassSkillDefinitions", document)
        self.assertTrue(ok, message)

    def test_integer_boundaries_still_follow_schema(self):
        fields = (
            ("ClassMagicDefinitions", ["classes", 0, "first_spell_level"], 99),
            ("ClassMagicDefinitions", ["classes", 0, "armour_weight_limit"], 999),
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0, "spells_info", 0, "learn_level"], 99),
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0, "spells_info", 0, "mana_cost"], 999),
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0, "spells_info", 0, "difficulty"], 999),
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0, "spells_info", 0, "first_cast_exp_rate"], 999),
            ("SpellDefinitions", ["realms", 0, "books", 0, "spells", 0, "spell_id"], 31),
        )
        for name, path, maximum in fields:
            for value, valid in ((0, True), (maximum, True), (-1, False), (maximum + 1, False)):
                with self.subTest(name=name, path=path, value=value):
                    self.check_value(name, path, value, valid)

    def test_nested_records_must_be_objects(self):
        fields = (
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0]),
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0, "spells_info", 0]),
            ("SpellDefinitions", ["realms", 0, "books", 0]),
            ("SpellDefinitions", ["realms", 0, "books", 0, "spells", 0]),
        )
        for name, path in fields:
            for value in (1, [], None):
                with self.subTest(name=name, path=path, value=value):
                    self.check_value(name, path, value, False, "object")

    def test_reader_accepted_empty_arrays_and_strings(self):
        magic = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        magic["classes"][0]["realms"][0]["spells_info"] = []
        spell = copy.deepcopy(self.samples["SpellDefinitions"])
        record = spell["realms"][0]["books"][0]["spells"][0]
        record["spell_tag"] = ""
        record["name"] = record["description"] = {"ja": "", "en": ""}
        for name, document in (("ClassMagicDefinitions", magic), ("SpellDefinitions", spell)):
            with self.subTest(name=name):
                ok, message = self.validate(name, document)
                self.assertTrue(ok, message)
        self.check_value("ClassMagicDefinitions", ["classes", 0, "realms"], [], True)
        self.check_value("SpellDefinitions", ["realms", 0, "books"], [], True)
        self.check_value("SpellDefinitions", ["realms", 0, "books", 0, "spells"], [], True)

    def test_version_policy_is_unchanged(self):
        for name in self.samples:
            with self.subTest(name=name):
                self.check_value(name, ["version"], 1.0, True)

    def test_repeated_spell_id_is_reader_accepted(self):
        document = copy.deepcopy(self.samples["SpellDefinitions"])
        spells = document["realms"][0]["books"][0]["spells"]
        spells.append(copy.deepcopy(spells[0]))
        spells[-1]["spell_tag"] = "replacement"
        ok, message = self.validate("SpellDefinitions", document)
        self.assertTrue(ok, message)

    def test_required_and_unknown_field_policy_is_unchanged(self):
        records = (
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0, "spells_info", 0], "learn_level"),
            ("ClassSkillDefinitions", ["classes", 0, "skills", "SHIELD"], "start_exp"),
            ("SpellDefinitions", ["realms", 0, "books", 0, "spells", 0], "spell_id"),
        )
        for name, path, required_field in records:
            for missing in (True, False):
                with self.subTest(name=name, missing=missing):
                    document = copy.deepcopy(self.samples[name])
                    record = document
                    for part in path:
                        record = record[part]
                    if missing:
                        del record[required_field]
                    else:
                        record["unknown"] = 1
                    ok, message = self.validate(name, document)
                    self.assertFalse(ok)
                    self.assertIn(f"Location: {path}", message)
                    self.assertIn("required property" if missing else "Additional properties", message)


if __name__ == "__main__":
    unittest.main()
