"""Reader integer and cross-field rules through the JSONC CI entry point."""
import copy
from pathlib import Path
import tempfile
import unittest

from json_validation_test_helper import validate_document
from validate_json import build_schema_registry, load_all_schemas, load_class_ids, load_jsonc


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

    def validate(self, name, document, spell_definitions=None):
        schema_path = self.schema_map[name]
        companions = {"SpellDefinitions.jsonc": spell_definitions} if spell_definitions is not None else None
        return validate_document(document, schema_path, self.loaded[schema_path], self.registry, companions=companions)

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

    def test_magic_class_order(self):
        document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
        document["classes"][0], document["classes"][1] = document["classes"][1], document["classes"][0]
        ok, message = self.validate("ClassMagicDefinitions", document)
        self.assertFalse(ok)
        self.assertIn("nondecreasing order", message)
        self.assertIn("Location: ['classes', 1, 'name']", message)

    def test_magic_repeated_classes_and_gaps_are_accepted(self):
        document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
        document["classes"] = [document["classes"][1], document["classes"][1], document["classes"][3]]
        ok, message = self.validate("ClassMagicDefinitions", document)
        self.assertTrue(ok, message)

    def test_class_ids_follow_enum_values_and_token_mapping(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / "src/player-info").mkdir(parents=True)
            (root / "src/info-reader").mkdir(parents=True)
            (root / "src/player-info/class-types.h").write_text(
                'enum class PlayerClassType : short { WARRIOR = 4, MAGE, MAX };', encoding="utf-8")
            (root / "src/info-reader/magic-reader.cpp").write_text(
                '{ "MAGE", PlayerClassType::MAGE },\n{ "FIGHTER", PlayerClassType::WARRIOR },', encoding="utf-8")
            self.assertEqual(load_class_ids(root), {"MAGE": 5, "FIGHTER": 4})

    def test_magic_unknown_spell_tag(self):
        self.check_value("ClassMagicDefinitions", ["classes", 0, "realms", 0, "spells_info", 0, "spell_tag"],
                         "NO_SUCH_TAG", False, "unknown spell tag in this realm")

    def test_magic_tag_from_another_realm_is_rejected(self):
        magic_realm = self.samples["ClassMagicDefinitions"]["classes"][0]["realms"][0]
        spells = self.documents["SpellDefinitions"]["realms"]
        own_tags = {spell["spell_tag"] for realm in spells if realm["name"] == magic_realm["name"]
                    for book in realm["books"] for spell in book["spells"]}
        foreign_tag = next(spell["spell_tag"] for realm in spells if realm["name"] != magic_realm["name"]
                           for book in realm["books"] for spell in book["spells"] if spell["spell_tag"] not in own_tags)
        self.check_value("ClassMagicDefinitions", ["classes", 0, "realms", 0, "spells_info", 0, "spell_tag"],
                         foreign_tag, False, "unknown spell tag in this realm")

    def test_magic_uses_adjacent_candidate_spell_definitions(self):
        magic = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        spells = copy.deepcopy(self.samples["SpellDefinitions"])
        spells["realms"][0]["name"] = magic["classes"][0]["realms"][0]["name"]
        spell = spells["realms"][0]["books"][0]["spells"][0]
        spell["spell_tag"] = "CANDIDATE_ONLY_TAG"
        magic["classes"][0]["realms"][0]["spells_info"][0]["spell_tag"] = spell["spell_tag"]
        ok, message = self.validate("ClassMagicDefinitions", magic, spells)
        self.assertTrue(ok, message)
        spell["spell_tag"] = "REPLACED_TAG"
        ok, message = self.validate("ClassMagicDefinitions", magic, spells)
        self.assertFalse(ok)
        self.assertIn("unknown spell tag in this realm", message)

    def test_magic_invalid_candidate_spell_definitions(self):
        for spells in ({"version": 1}, {"version": 1, "realms": [None]}):
            with self.subTest(spells=spells):
                ok, message = self.validate("ClassMagicDefinitions", self.samples["ClassMagicDefinitions"], spells)
                self.assertFalse(ok)
                self.assertIn("Invalid spell definitions", message)
                self.assertIn("SpellDefinitions.jsonc", message)

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

    def test_repeated_spell_id_is_rejected(self):
        document = copy.deepcopy(self.samples["SpellDefinitions"])
        spells = document["realms"][0]["books"][0]["spells"]
        spells.append(copy.deepcopy(spells[0]))
        spells[-1]["spell_tag"] = "replacement"
        ok, message = self.validate("SpellDefinitions", document)
        self.assertFalse(ok)
        self.assertIn("spell IDs must be unique within each realm", message)
        self.assertIn("Location: ['realms', 0, 'books', 0, 'spells', 1, 'spell_id']", message)

    def test_repeated_spell_id_across_books_is_rejected(self):
        document = copy.deepcopy(self.samples["SpellDefinitions"])
        books = document["realms"][0]["books"]
        books.append(copy.deepcopy(books[0]))
        ok, message = self.validate("SpellDefinitions", document)
        self.assertFalse(ok)
        self.assertIn("Location: ['realms', 0, 'books', 1, 'spells', 0, 'spell_id']", message)

    def test_repeated_spell_id_across_same_realm_records_is_rejected(self):
        document = copy.deepcopy(self.samples["SpellDefinitions"])
        document["realms"].append(copy.deepcopy(document["realms"][0]))
        ok, message = self.validate("SpellDefinitions", document)
        self.assertFalse(ok)
        self.assertIn("Location: ['realms', 1, 'books', 0, 'spells', 0, 'spell_id']", message)

    def test_same_spell_id_in_different_realms_is_accepted(self):
        document = copy.deepcopy(self.samples["SpellDefinitions"])
        document["realms"].append(copy.deepcopy(document["realms"][0]))
        document["realms"][1]["name"] = "SORCERY"
        ok, message = self.validate("SpellDefinitions", document)
        self.assertTrue(ok, message)

    def test_repeated_spell_tag_with_different_ids_is_rejected(self):
        for tag in ("DUPLICATE_TAG", ""):
            with self.subTest(tag=tag):
                document = copy.deepcopy(self.samples["SpellDefinitions"])
                spells = document["realms"][0]["books"][0]["spells"]
                spells[0]["spell_tag"] = tag
                spells.append(copy.deepcopy(spells[0]))
                spells[-1]["spell_id"] += 1
                ok, message = self.validate("SpellDefinitions", document)
                self.assertFalse(ok)
                self.assertIn("spell tags must be unique within each realm", message)
                self.assertIn("Location: ['realms', 0, 'books', 0, 'spells', 1, 'spell_tag']", message)

    def test_repeated_spell_tag_across_books_is_rejected(self):
        document = copy.deepcopy(self.samples["SpellDefinitions"])
        books = document["realms"][0]["books"]
        books.append(copy.deepcopy(books[0]))
        books[-1]["spells"][0]["spell_id"] += 1
        ok, message = self.validate("SpellDefinitions", document)
        self.assertFalse(ok)
        self.assertIn("spell tags must be unique within each realm", message)
        self.assertIn("Location: ['realms', 0, 'books', 1, 'spells', 0, 'spell_tag']", message)

    def test_repeated_spell_tag_across_same_realm_records_is_rejected(self):
        document = copy.deepcopy(self.samples["SpellDefinitions"])
        document["realms"].append(copy.deepcopy(document["realms"][0]))
        document["realms"][-1]["books"][0]["spells"][0]["spell_id"] += 1
        ok, message = self.validate("SpellDefinitions", document)
        self.assertFalse(ok)
        self.assertIn("spell tags must be unique within each realm", message)
        self.assertIn("Location: ['realms', 1, 'books', 0, 'spells', 0, 'spell_tag']", message)

    def test_same_spell_tag_with_different_ids_in_different_realms_is_accepted(self):
        document = copy.deepcopy(self.samples["SpellDefinitions"])
        document["realms"].append(copy.deepcopy(document["realms"][0]))
        document["realms"][1]["name"] = "SORCERY"
        document["realms"][1]["books"][0]["spells"][0]["spell_id"] += 1
        ok, message = self.validate("SpellDefinitions", document)
        self.assertTrue(ok, message)

    def test_magic_rejects_ambiguous_spell_tags_before_reference_lookup(self):
        magic = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        spells = copy.deepcopy(self.samples["SpellDefinitions"])
        spells["realms"][0]["name"] = magic["classes"][0]["realms"][0]["name"]
        records = spells["realms"][0]["books"][0]["spells"]
        magic["classes"][0]["realms"][0]["spells_info"][0]["spell_tag"] = records[0]["spell_tag"]
        ok, message = self.validate("ClassMagicDefinitions", magic, spells)
        self.assertTrue(ok, message)
        records.append(copy.deepcopy(records[0]))
        records[-1]["spell_id"] += 1
        ok, message = self.validate("ClassMagicDefinitions", magic, spells)
        self.assertFalse(ok)
        self.assertIn("Invalid spell definitions", message)
        self.assertIn("spell tags must be unique within each realm", message)
        self.assertIn("Location: ['realms', 0, 'books', 0, 'spells', 1, 'spell_tag']", message)

    def test_magic_empty_tag_with_undefined_lower_id_is_rejected(self):
        for spell_id in (1, 31):
            with self.subTest(spell_id=spell_id):
                magic = copy.deepcopy(self.samples["ClassMagicDefinitions"])
                spells = copy.deepcopy(self.samples["SpellDefinitions"])
                spells["realms"][0]["name"] = magic["classes"][0]["realms"][0]["name"]
                spell = spells["realms"][0]["books"][0]["spells"][0]
                spell["spell_id"] = spell_id
                spell["spell_tag"] = ""
                magic["classes"][0]["realms"][0]["spells_info"][0]["spell_tag"] = ""
                ok, message = self.validate("SpellDefinitions", spells)
                self.assertTrue(ok, message)
                ok, message = self.validate("ClassMagicDefinitions", magic, spells)
                self.assertFalse(ok)
                self.assertIn("unknown spell tag in this realm", message)
                self.assertIn("Location: ['classes', 0, 'realms', 0, 'spells_info', 0, 'spell_tag']", message)

    def test_magic_empty_tag_without_undefined_lower_ids_is_accepted(self):
        for spell_id in (0, 2):
            with self.subTest(spell_id=spell_id):
                magic = copy.deepcopy(self.samples["ClassMagicDefinitions"])
                spells = copy.deepcopy(self.samples["SpellDefinitions"])
                spells["realms"][0]["name"] = magic["classes"][0]["realms"][0]["name"]
                books = spells["realms"][0]["books"]
                spell = books[0]["spells"][0]
                spell["spell_id"] = spell_id
                spell["spell_tag"] = ""
                books.append(copy.deepcopy(books[0]))
                books[-1]["spells"] = []
                for lower_id in range(spell_id):
                    lower = copy.deepcopy(spell)
                    lower["spell_id"] = lower_id
                    lower["spell_tag"] = f"LOWER_{lower_id}"
                    books[-1]["spells"].append(lower)
                magic["classes"][0]["realms"][0]["spells_info"][0]["spell_tag"] = ""
                ok, message = self.validate("ClassMagicDefinitions", magic, spells)
                self.assertTrue(ok, message)

    def test_magic_empty_tag_checks_lower_ids_across_same_realm_records(self):
        magic = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        spells = copy.deepcopy(self.samples["SpellDefinitions"])
        spells["realms"][0]["name"] = magic["classes"][0]["realms"][0]["name"]
        spell = spells["realms"][0]["books"][0]["spells"][0]
        spell["spell_id"] = 2
        spell["spell_tag"] = ""
        spells["realms"].append(copy.deepcopy(spells["realms"][0]))
        lower_spells = spells["realms"][1]["books"][0]["spells"]
        lower_spells.clear()
        for lower_id in range(2):
            lower = copy.deepcopy(spell)
            lower["spell_id"] = lower_id
            lower["spell_tag"] = f"LOWER_{lower_id}"
            lower_spells.append(lower)
        magic["classes"][0]["realms"][0]["spells_info"][0]["spell_tag"] = ""
        ok, message = self.validate("ClassMagicDefinitions", magic, spells)
        self.assertTrue(ok, message)
        lower_spells.pop()
        ok, message = self.validate("ClassMagicDefinitions", magic, spells)
        self.assertFalse(ok)
        self.assertIn("unknown spell tag in this realm", message)

    def test_magic_rejects_spell_overwrite_before_reference_lookup(self):
        spells = copy.deepcopy(self.documents["SpellDefinitions"])
        records = spells["realms"][0]["books"][0]["spells"]
        records.append(copy.deepcopy(records[0]))
        records[-1]["spell_tag"] = "REPLACEMENT_TAG"
        ok, message = self.validate("ClassMagicDefinitions", self.samples["ClassMagicDefinitions"], spells)
        self.assertFalse(ok)
        self.assertIn("Invalid spell definitions", message)
        self.assertIn("spell IDs must be unique within each realm", message)

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
