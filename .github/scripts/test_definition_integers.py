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
        # 値を自由に変えられる職業を見本にする (MAGE の値は SpellDefinitions の基準値と同じでなければならない)。
        magic["classes"] = [record for record in magic["classes"] if record["name"] == "PRIEST"]
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
        # 習得できる巻の魔法はすべて書く決まりなので、見本の職業魔法は見本の魔法の定義と組で検査する
        assert magic["classes"][0]["realms"][0]["name"] == spell["realms"][0]["name"]
        assert magic["classes"][0]["realms"][0]["spells_info"][0]["spell_tag"] == spell["realms"][0]["books"][0]["spells"][0]["spell_tag"]

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
        companion = self.samples["SpellDefinitions"] if name == "ClassMagicDefinitions" else None
        ok, message = self.validate(name, document, companion)
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

    def test_spell_integer_base_values(self):
        spell_path = ["realms", 0, "books", 0, "spells", 0]
        for field in ("learn_level", "mana_cost", "difficulty", "first_cast_exp_rate"):
            with self.subTest(field=field):
                self.check_value("SpellDefinitions", spell_path + [field], 1.0, False, "integer JSON value")
                for value in (True, "1", None, 1.5):
                    self.check_value("SpellDefinitions", spell_path + [field], value, False)

    def test_spell_base_value_boundaries(self):
        spell_path = ["realms", 0, "books", 0, "spells", 0]
        fields = (("learn_level", 1, 50), ("mana_cost", 1, 2047), ("difficulty", 0, 100), ("first_cast_exp_rate", 0, 2047))
        for field, minimum, maximum in fields:
            for value, valid in ((minimum, True), (maximum, True), (minimum - 1, False), (maximum + 1, False)):
                with self.subTest(field=field, value=value):
                    self.check_value("SpellDefinitions", spell_path + [field], value, valid)

    def test_spell_base_values_are_required(self):
        for field in ("learn_level", "mana_cost", "difficulty", "first_cast_exp_rate"):
            with self.subTest(field=field):
                document = copy.deepcopy(self.samples["SpellDefinitions"])
                del document["realms"][0]["books"][0]["spells"][0][field]
                ok, message = self.validate("SpellDefinitions", document)
                self.assertFalse(ok, message)

    def test_mage_values_must_match_spell_base_values(self):
        fields = ("learn_level", "mana_cost", "difficulty", "first_cast_exp_rate")
        mage_index = next(index for index, record in enumerate(self.documents["ClassMagicDefinitions"]["classes"])
                          if record["name"] == "MAGE")
        for field in fields:
            with self.subTest(field=field):
                document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
                spell = document["classes"][mage_index]["realms"][0]["spells_info"][0]
                spell[field] += 1
                ok, message = self.validate("ClassMagicDefinitions", document)
                self.assertFalse(ok, message)
                self.assertIn("MAGE must have the base value", message)
                self.assertIn(f"Location: ['classes', {mage_index}, 'realms', 0, 'spells_info', 0, '{field}']", message)

    def test_spell_base_values_must_match_mage_values(self):
        # 基準値の側を変えても、MAGE の値との食い違いとして止まる。
        spells = copy.deepcopy(self.documents["SpellDefinitions"])
        spells["realms"][0]["books"][0]["spells"][0]["mana_cost"] += 1
        ok, message = self.validate("ClassMagicDefinitions", self.documents["ClassMagicDefinitions"], spells)
        self.assertFalse(ok, message)
        self.assertIn("MAGE must have the base value", message)

    def test_other_class_values_may_differ_from_spell_base_values(self):
        document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        document["classes"][0]["realms"][0]["spells_info"][0]["mana_cost"] += 1
        ok, message = self.validate("ClassMagicDefinitions", document, self.samples["SpellDefinitions"])
        self.assertTrue(ok, message)

    def test_technic_base_values_may_be_changed(self):
        # 歌・武芸・呪術は基準値そのものをゲームが使うので、範囲内なら変えてよい。
        realms = self.documents["SpellDefinitions"]["realms"]
        for name in ("MUSIC", "HISSATSU", "HEX"):
            realm_index = next(index for index, realm in enumerate(realms) if realm["name"] == name)
            for field in ("learn_level", "mana_cost", "difficulty", "first_cast_exp_rate"):
                with self.subTest(realm=name, field=field):
                    document = copy.deepcopy(self.documents["SpellDefinitions"])
                    # 各領域の 2 巻目の先頭は、どの項目も 1 足して範囲に収まる。
                    document["realms"][realm_index]["books"][1]["spells"][0][field] += 1
                    ok, message = self.validate("SpellDefinitions", document)
                    self.assertTrue(ok, message)

    def test_spell_version_must_be_integer_two(self):
        for value, valid in ((2, True), (2.0, False), (1, False), (1.0, False), (3, False), ("2", False)):
            with self.subTest(value=value):
                document = copy.deepcopy(self.samples["SpellDefinitions"])
                document["version"] = value
                ok, message = self.validate("SpellDefinitions", document)
                self.assertEqual(ok, valid, message)
                if not valid:
                    self.assertIn("Location: ['version']", message)

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
        self.assertIn("strictly increasing order", message)
        self.assertIn("Location: ['classes', 1, 'name']", message)

    def test_magic_gaps_are_accepted(self):
        document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
        document["classes"] = [document["classes"][1], document["classes"][3]]
        ok, message = self.validate("ClassMagicDefinitions", document)
        self.assertTrue(ok, message)

    def test_magic_repeated_class_is_rejected(self):
        document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
        document["classes"] = [document["classes"][1], document["classes"][1]]
        ok, message = self.validate("ClassMagicDefinitions", document)
        self.assertFalse(ok)
        self.assertIn("strictly increasing order", message)
        self.assertIn("Location: ['classes', 1, 'name']", message)

    def test_class_ids_follow_enum_values_and_token_mapping(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / "src/player-info").mkdir(parents=True)
            (root / "src/definitions-reader").mkdir(parents=True)
            (root / "src/player-info/class-types.h").write_text(
                'enum class PlayerClassType : short { WARRIOR = 4, MAGE, MAX };', encoding="utf-8")
            (root / "src/definitions-reader/magic-tokens-table.cpp").write_text(
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

    def red_mage_realm(self, document, name):
        record_index = next(index for index, record in enumerate(document["classes"]) if record["name"] == "RED_MAGE")
        realm_index = next(index for index, realm in enumerate(document["classes"][record_index]["realms"]) if realm["name"] == name)
        return ["classes", record_index, "realms", realm_index], document["classes"][record_index]["realms"][realm_index]

    def test_magic_max_book_range(self):
        path = ["classes", 0, "realms", 0, "max_book"]
        for value, accepted in ((0, False), (1, True), (3, True), (5, False), (1.0, False), ("2", False)):
            with self.subTest(value=value):
                self.check_value("ClassMagicDefinitions", path, value, accepted)

    def test_magic_max_book_default_must_be_omitted(self):
        self.check_value("ClassMagicDefinitions", ["classes", 0, "realms", 0, "max_book"], 4, False, "must be omitted")

    def test_magic_spells_after_max_book_must_not_be_listed(self):
        # 赤魔道師の仙術は 2 冊目まで。3 冊目の先頭 (呪文 ID 16) は spells_info にも unlearnable にも書けない
        book3_tag = self.spell_tag("SORCERY", 16)
        for field in ("spells_info", "unlearnable"):
            with self.subTest(field=field):
                document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
                realm_path, realm = self.red_mage_realm(document, "SORCERY")
                self.assertEqual(realm["max_book"], 2)
                if field == "spells_info":
                    realm["spells_info"].append({"spell_tag": book3_tag, "learn_level": 50, "mana_cost": 1, "difficulty": 0, "first_cast_exp_rate": 0})
                    location = realm_path + ["spells_info", len(realm["spells_info"]) - 1, "spell_tag"]
                else:
                    realm["unlearnable"] = [book3_tag]
                    location = realm_path + ["unlearnable", 0]
                ok, message = self.validate("ClassMagicDefinitions", document)
                self.assertFalse(ok, message)
                self.assertIn("spells after book 2 are unlearnable and must not be listed", message)
                self.assertIn(f"Location: {location}", message)

    def test_magic_max_book_allows_learnable_spells_up_to_the_book(self):
        document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
        _, realm = self.red_mage_realm(document, "ARCANE")
        # 秘術は 4 冊目まで書いてあるので、3 冊目までにするとはみ出す。4 冊目を消せば通る
        realm["max_book"] = 3
        ok, message = self.validate("ClassMagicDefinitions", document)
        self.assertFalse(ok, message)
        book4_tags = {self.spell_tag("ARCANE", spell_id) for spell_id in range(24, 32)}
        realm["spells_info"] = [spell for spell in realm["spells_info"] if spell["spell_tag"] not in book4_tags]
        ok, message = self.validate("ClassMagicDefinitions", document)
        self.assertTrue(ok, message)

    def rogue_sorcery(self, document):
        record_index = next(index for index, record in enumerate(document["classes"]) if record["name"] == "ROGUE")
        realm_index = next(index for index, realm in enumerate(document["classes"][record_index]["realms"]) if realm["name"] == "SORCERY")
        return ["classes", record_index, "realms", realm_index], document["classes"][record_index]["realms"][realm_index]

    def test_magic_unlearnable_spells_are_listed_once(self):
        _, realm = self.rogue_sorcery(self.documents["ClassMagicDefinitions"])
        self.assertEqual(realm["unlearnable"], ["Globe_of_Invulnerability"])
        learnable_tag = realm["spells_info"][0]["spell_tag"]
        cases = (
            ("in both", ["Globe_of_Invulnerability", learnable_tag], ["unlearnable", 1], "spell is listed twice"),
            ("twice", ["Globe_of_Invulnerability", "Globe_of_Invulnerability"], ["unlearnable"], "non-unique elements"),
            ("unknown", ["Globe_of_Invulnerability", "Unknown_Spell"], ["unlearnable", 1], "unknown spell tag in this realm"),
            ("not listed", [], [], "spell Globe_of_Invulnerability must be listed in spells_info or unlearnable"),
        )
        for name, unlearnable, location, reason in cases:
            with self.subTest(name=name):
                document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
                realm_path, realm = self.rogue_sorcery(document)
                realm["unlearnable"] = unlearnable
                ok, message = self.validate("ClassMagicDefinitions", document)
                self.assertFalse(ok, message)
                self.assertIn(reason, message)
                self.assertIn(f"Location: {realm_path + location}", message)

    def test_magic_spells_info_spell_listed_twice(self):
        document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        spells_info = document["classes"][0]["realms"][0]["spells_info"]
        spells_info.append(copy.deepcopy(spells_info[0]))
        ok, message = self.validate("ClassMagicDefinitions", document, self.samples["SpellDefinitions"])
        self.assertFalse(ok, message)
        self.assertIn("spell is listed twice", message)
        self.assertIn(f"Location: ['classes', 0, 'realms', 0, 'spells_info', {len(spells_info) - 1}, 'spell_tag']", message)

    def spell_tag(self, realm_name, spell_id):
        realm = next(realm for realm in self.documents["SpellDefinitions"]["realms"] if realm["name"] == realm_name)
        return next(spell["spell_tag"] for book in realm["books"] for spell in book["spells"] if spell["spell_id"] == spell_id)

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
            ("ClassMagicDefinitions", ["classes", 0, "armour_weight_limit"], 2047),
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0, "spells_info", 0, "mana_cost"], 2047),
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0, "spells_info", 0, "difficulty"], 100),
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0, "spells_info", 0, "first_cast_exp_rate"], 2047),
            ("SpellDefinitions", ["realms", 0, "books", 0, "spells", 0, "spell_id"], 31),
        )
        for name, path, maximum in fields:
            for value, valid in ((0, True), (maximum, True), (-1, False), (maximum + 1, False)):
                with self.subTest(name=name, path=path, value=value):
                    self.check_value(name, path, value, valid)

    def test_magic_learn_level_boundaries(self):
        # 習得できない魔法は unlearnable に書くので、習得レベルは 1〜50 だけ
        path = ["classes", 0, "realms", 0, "spells_info", 0, "learn_level"]
        for value, valid in ((1, True), (50, True), (0, False), (51, False), (99, False)):
            with self.subTest(value=value):
                self.check_value("ClassMagicDefinitions", path, value, valid)

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
        no_spells = copy.deepcopy(self.samples["SpellDefinitions"])
        no_spells["realms"][0]["books"] = []
        for name, document, companion in (("ClassMagicDefinitions", magic, no_spells), ("SpellDefinitions", spell, None)):
            with self.subTest(name=name):
                ok, message = self.validate(name, document, companion)
                self.assertTrue(ok, message)
        self.check_value("ClassMagicDefinitions", ["classes", 0, "realms"], [], True)
        self.check_value("SpellDefinitions", ["realms", 0, "books"], [], True)
        self.check_value("SpellDefinitions", ["realms", 0, "books", 0, "spells"], [], True)

    def test_version_policy_is_unchanged(self):
        # SpellDefinitions は reader が version を確かめる (test_spell_version_must_be_integer_two)。
        for name in ("ClassMagicDefinitions", "ClassSkillDefinitions"):
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
                magic["classes"][0]["realms"][0]["unlearnable"] = [f"LOWER_{lower_id}" for lower_id in range(spell_id)]
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
        magic["classes"][0]["realms"][0]["unlearnable"] = ["LOWER_0", "LOWER_1"]
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
