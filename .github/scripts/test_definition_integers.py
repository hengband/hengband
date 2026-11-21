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
        # 生命領域を第一・第二の両方に選べる職業を見本にする (選べる領域はすべて書く決まりなので、領域は減らさない)。
        magic["classes"] = [record for record in magic["classes"] if record["name"] == "PRIEST"]
        spell = copy.deepcopy(cls.documents["SpellDefinitions"])
        spell["realms"] = spell["realms"][:1]
        spell["realms"][0]["books"] = spell["realms"][0]["books"][:1]
        spell["realms"][0]["books"][0]["spells"] = spell["realms"][0]["books"][0]["spells"][:1]
        cls.samples = {
            "ClassMagicDefinitions": magic,
            "ClassSkillDefinitions": cls.documents["ClassSkillDefinitions"],
            "SpellDefinitions": spell,
        }
        # 見本の職業魔法は見本の魔法の定義と組で検査する。先頭の領域の段階を (2, 1) にし、見本の魔法に段階の上書きを 1 つ書いておく
        realm = magic["classes"][0]["realms"][0]
        assert realm["name"] == spell["realms"][0]["name"]
        realm["aptitude"] = 2
        realm["aptitude_as_second"] = 1
        realm["overrides"] = [{"spell_tag": spell["realms"][0]["books"][0]["spells"][0]["spell_tag"], "aptitude": 0}]

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
        realm_path = class_path + ["realms", 0]
        paths = [(class_path + [field]) for field in ("first_spell_level", "armour_weight_limit")]
        paths += [(realm_path + [field]) for field in ("aptitude", "aptitude_as_second")]
        paths += [realm_path + ["overrides", 0, "aptitude"]]
        paths += [["aptitude_steps", field] for field in ("learn_level_percent", "mana_cost_percent", "difficulty")]
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

    def test_spell_base_values_may_be_changed(self):
        # 職業別の値は基準値から求めるので、基準値を変えても職業魔法の定義はそのまま通る。
        spells = copy.deepcopy(self.documents["SpellDefinitions"])
        spells["realms"][0]["books"][0]["spells"][0]["mana_cost"] += 1
        ok, message = self.validate("ClassMagicDefinitions", self.documents["ClassMagicDefinitions"], spells)
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

    def test_magic_version_must_be_integer_two(self):
        for value, valid in ((2, True), (2.0, False), (1, False), (1.0, False), (3, False), ("2", False)):
            with self.subTest(value=value):
                self.check_value("ClassMagicDefinitions", ["version"], value, valid)

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
        for field, path in (("overrides", ["overrides", 0, "spell_tag"]), ("unlearnable", ["unlearnable", 0])):
            with self.subTest(field=field):
                document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
                document["classes"][0]["realms"][0][field] = [{"spell_tag": "NO_SUCH_TAG", "aptitude": 0}] if field == "overrides" else ["NO_SUCH_TAG"]
                ok, message = self.validate("ClassMagicDefinitions", document, self.samples["SpellDefinitions"])
                self.assertFalse(ok, message)
                self.assertIn("unknown spell tag in this realm", message)
                self.assertIn(f"Location: {['classes', 0, 'realms', 0] + path}", message)
    def test_magic_tag_from_another_realm_is_rejected(self):
        magic_realm = self.samples["ClassMagicDefinitions"]["classes"][0]["realms"][0]
        spells = self.documents["SpellDefinitions"]["realms"]
        own_tags = {spell["spell_tag"] for realm in spells if realm["name"] == magic_realm["name"]
                    for book in realm["books"] for spell in book["spells"]}
        foreign_tag = next(spell["spell_tag"] for realm in spells if realm["name"] != magic_realm["name"]
                           for book in realm["books"] for spell in book["spells"] if spell["spell_tag"] not in own_tags)
        self.check_value("ClassMagicDefinitions", ["classes", 0, "realms", 0, "overrides", 0, "spell_tag"],
                         foreign_tag, False, "unknown spell tag in this realm")

    def test_magic_uses_adjacent_candidate_spell_definitions(self):
        magic = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        spells = copy.deepcopy(self.samples["SpellDefinitions"])
        spells["realms"][0]["name"] = magic["classes"][0]["realms"][0]["name"]
        spell = spells["realms"][0]["books"][0]["spells"][0]
        spell["spell_tag"] = "CANDIDATE_ONLY_TAG"
        magic["classes"][0]["realms"][0]["overrides"][0]["spell_tag"] = spell["spell_tag"]
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
        # 赤魔道師の仙術は 2 冊目まで。3 冊目の先頭 (呪文 ID 16) は overrides にも unlearnable にも書けない
        book3_tag = self.spell_tag("SORCERY", 16)
        for field in ("overrides", "unlearnable"):
            with self.subTest(field=field):
                document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
                realm_path, realm = self.red_mage_realm(document, "SORCERY")
                self.assertEqual(realm["max_book"], 2)
                if field == "overrides":
                    realm["overrides"] = [{"spell_tag": book3_tag, "aptitude": 0}]
                    location = realm_path + ["overrides", 0, "spell_tag"]
                else:
                    realm["unlearnable"] = [book3_tag]
                    location = realm_path + ["unlearnable", 0]
                ok, message = self.validate("ClassMagicDefinitions", document)
                self.assertFalse(ok, message)
                self.assertIn("spells after book 2 are unlearnable and must not be listed", message)
                self.assertIn(f"Location: {location}", message)
    def test_magic_max_book_needs_no_spell_list(self):
        # 習得できる巻の魔法は書かなくても基準値から求めるので、巻数を変えるだけで通る
        document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
        _, realm = self.red_mage_realm(document, "ARCANE")
        realm["max_book"] = 3
        ok, message = self.validate("ClassMagicDefinitions", document)
        self.assertTrue(ok, message)
    def rogue_sorcery(self, document):
        record_index = next(index for index, record in enumerate(document["classes"]) if record["name"] == "ROGUE")
        realm_index = next(index for index, realm in enumerate(document["classes"][record_index]["realms"]) if realm["name"] == "SORCERY")
        return ["classes", record_index, "realms", realm_index], document["classes"][record_index]["realms"][realm_index]

    def test_magic_unlearnable_spells_are_listed_once(self):
        _, realm = self.rogue_sorcery(self.documents["ClassMagicDefinitions"])
        self.assertEqual(realm["unlearnable"], ["Globe_of_Invulnerability"])
        cases = (
            ("twice", ["Globe_of_Invulnerability", "Globe_of_Invulnerability"], ["unlearnable"], "non-unique elements"),
            ("unknown", ["Globe_of_Invulnerability", "Unknown_Spell"], ["unlearnable", 1], "unknown spell tag in this realm"),
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

        # 習得できない魔法に段階の上書きは書けない
        document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
        realm_path, realm = self.rogue_sorcery(document)
        realm["overrides"] = [{"spell_tag": "Globe_of_Invulnerability", "aptitude": 0}]
        ok, message = self.validate("ClassMagicDefinitions", document)
        self.assertFalse(ok, message)
        self.assertIn("spell is listed twice", message)
        self.assertIn(f"Location: {realm_path + ['overrides', 0, 'spell_tag']}", message)

        # 一覧を空にすると、その魔法も基準値から求めて習得できる
        document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
        _, realm = self.rogue_sorcery(document)
        del realm["unlearnable"]
        ok, message = self.validate("ClassMagicDefinitions", document)
        self.assertTrue(ok, message)
    def test_magic_override_listed_twice(self):
        document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        overrides = document["classes"][0]["realms"][0]["overrides"]
        overrides.append({"spell_tag": overrides[0]["spell_tag"], "aptitude": -1})
        ok, message = self.validate("ClassMagicDefinitions", document, self.samples["SpellDefinitions"])
        self.assertFalse(ok, message)
        self.assertIn("spell is listed twice", message)
        self.assertIn("Location: ['classes', 0, 'realms', 0, 'overrides', 1, 'spell_tag']", message)

    def test_magic_redundant_aptitudes_are_rejected(self):
        realm_path = ["classes", 0, "realms", 0]
        # 第二領域の段階が第一領域と同じなら書かない
        self.check_value("ClassMagicDefinitions", realm_path + ["aptitude_as_second"], 2, False, "must be omitted")
        # 上書きの段階が第一・第二の両方と同じなら書かない (どちらか一方と同じなら意味がある)
        document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        document["classes"][0]["realms"][0]["aptitude_as_second"] = 0
        ok, message = self.validate("ClassMagicDefinitions", document, self.samples["SpellDefinitions"])
        self.assertTrue(ok, message)
        del document["classes"][0]["realms"][0]["aptitude_as_second"]
        document["classes"][0]["realms"][0]["aptitude"] = 0
        ok, message = self.validate("ClassMagicDefinitions", document, self.samples["SpellDefinitions"])
        self.assertFalse(ok, message)
        self.assertIn("same as the realm and must be omitted", message)
        self.assertIn(f"Location: {realm_path + ['overrides', 0, 'aptitude']}", message)

    def test_magic_realm_listed_twice(self):
        document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        realms = document["classes"][0]["realms"]
        realms.append({"name": realms[0]["name"], "aptitude": -1})
        ok, message = self.validate("ClassMagicDefinitions", document, self.samples["SpellDefinitions"])
        self.assertFalse(ok, message)
        self.assertIn("realm is listed twice", message)
        self.assertIn(f"Location: {['classes', 0, 'realms', len(realms) - 1, 'name']}", message)

    def test_magic_realms_must_match_the_class_choices(self):
        cases = (
            # プリーストから選べる領域を消す
            ("PRIEST", lambda realms: realms.pop(0), "realm LIFE that PRIEST can choose must be listed", []),
            # 混沌の戦士はカオスと悪魔だけを選べる
            ("CHAOS_WARRIOR", lambda realms: realms.append({"name": "LIFE"}), "realm LIFE cannot be chosen by CHAOS_WARRIOR", [2, "name"]),
            # 全領域を使う職業は 10 領域すべて
            ("SORCERER", lambda realms: realms.pop(), "realm CRUSADE that SORCERER can choose must be listed", []),
            # 魔法を使わない職業は書かない
            ("WARRIOR", lambda realms: realms.append({"name": "LIFE"}), "realm LIFE cannot be chosen by WARRIOR", [0, "name"]),
        )
        for name, change, reason, location in cases:
            with self.subTest(name=name):
                document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
                class_index = next(index for index, record in enumerate(document["classes"]) if record["name"] == name)
                change(document["classes"][class_index]["realms"])
                ok, message = self.validate("ClassMagicDefinitions", document)
                self.assertFalse(ok, message)
                self.assertIn(reason, message)
                self.assertIn(f"Location: {['classes', class_index, 'realms'] + location}", message)

    def test_magic_aptitude_as_second_needs_both_positions(self):
        # レンジャーの自然は第一領域だけ、プリーストの生命は第一・第二の両方に選べる
        document = copy.deepcopy(self.documents["ClassMagicDefinitions"])
        class_index = next(index for index, record in enumerate(document["classes"]) if record["name"] == "RANGER")
        realms = document["classes"][class_index]["realms"]
        realm_index = next(index for index, realm in enumerate(realms) if realm["name"] == "NATURE")
        realms[realm_index]["aptitude_as_second"] = realms[realm_index].get("aptitude", 0) + 1
        ok, message = self.validate("ClassMagicDefinitions", document)
        self.assertFalse(ok, message)
        self.assertIn("aptitude_as_second is allowed only for realms that can be both", message)
        self.assertIn(f"Location: {['classes', class_index, 'realms', realm_index, 'aptitude_as_second']}", message)

    def test_magic_derived_values_of_spells_with_empty_tags(self):
        # 呪文タグが空の魔法も値を求める (段階 7 では消費MP の倍率が負になる)
        spells = copy.deepcopy(self.samples["SpellDefinitions"])
        spells["realms"][0]["books"][0]["spells"][0]["spell_tag"] = ""
        document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        realm = document["classes"][0]["realms"][0]
        del realm["overrides"]
        realm["aptitude"] = 7
        ok, message = self.validate("ClassMagicDefinitions", document, spells)
        self.assertFalse(ok, message)
        self.assertIn("derived values of spell 0 () are out of range at aptitude 7", message)

    def test_magic_aptitude_ranges(self):
        # 消費MP を動かさない傾きで、段階の範囲 (-9〜9) だけを確かめる
        for field in ("aptitude", "aptitude_as_second"):
            for value, valid in ((-9, True), (9, True), (-10, False), (10, False)):
                with self.subTest(field=field, value=value):
                    document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
                    document["aptitude_steps"]["mana_cost_percent"] = 0
                    realm = document["classes"][0]["realms"][0]
                    realm[field] = value
                    if realm["aptitude"] == realm["aptitude_as_second"]:
                        realm["aptitude_as_second"] = 0 if value != 0 else 1
                    ok, message = self.validate("ClassMagicDefinitions", document, self.samples["SpellDefinitions"])
                    self.assertEqual(ok, valid, message)

    def test_magic_aptitude_steps_ranges(self):
        for field, maximum in (("learn_level_percent", 100), ("mana_cost_percent", 50), ("difficulty", 20)):
            for value, valid in ((0, True), (maximum, True), (-1, False), (maximum + 1, False)):
                with self.subTest(field=field, value=value):
                    document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
                    document["aptitude_steps"][field] = value
                    ok, message = self.validate("ClassMagicDefinitions", document, self.samples["SpellDefinitions"])
                    self.assertEqual(ok, valid, message)
        document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        del document["aptitude_steps"]
        ok, message = self.validate("ClassMagicDefinitions", document, self.samples["SpellDefinitions"])
        self.assertFalse(ok, message)

    def test_magic_derived_values_out_of_range(self):
        # 段階 7 は消費MP の倍率が 100 - 15 × 7 = -5% になり、MagicReader は値を求められない
        # (見本の魔法は上書きで段階 0 にしてあるので、上書きを外して領域の段階を効かせる)
        for aptitude, valid in ((7, False), (6, True)):
            with self.subTest(aptitude=aptitude):
                document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
                realm = document["classes"][0]["realms"][0]
                del realm["overrides"]
                realm["aptitude"] = aptitude
                ok, message = self.validate("ClassMagicDefinitions", document, self.samples["SpellDefinitions"])
                self.assertEqual(ok, valid, message)
                if not valid:
                    self.assertIn("out of range at aptitude 7", message)
        # 消費MP が上限 2047 を超える基準値と段階の組
        spells = copy.deepcopy(self.samples["SpellDefinitions"])
        spells["realms"][0]["books"][0]["spells"][0]["mana_cost"] = 2047
        document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        document["classes"][0]["realms"][0]["overrides"][0]["aptitude"] = -1
        ok, message = self.validate("ClassMagicDefinitions", document, spells)
        self.assertFalse(ok, message)
        self.assertIn("out of range at aptitude -1", message)
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
            ("ClassMagicDefinitions", ["classes", 0, "first_spell_level"], 50),
            ("ClassMagicDefinitions", ["classes", 0, "armour_weight_limit"], 2047),
            ("SpellDefinitions", ["realms", 0, "books", 0, "spells", 0, "spell_id"], 31),
        )
        for name, path, maximum in fields:
            for value, valid in ((0, True), (maximum, True), (-1, False), (maximum + 1, False)):
                with self.subTest(name=name, path=path, value=value):
                    self.check_value(name, path, value, valid)

    def test_magic_first_spell_level_is_omitted_only_without_spellbooks(self):
        document = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        del document["classes"][0]["first_spell_level"]
        ok, message = self.validate("ClassMagicDefinitions", document, self.samples["SpellDefinitions"])
        self.assertFalse(ok, message)
        self.assertIn("first_spell_level is required for classes with spellbooks", message)
        self.assertIn("Location: ['classes', 0]", message)
        # 魔法を使わない職業は省略する (戦士は選べる領域が無いので、領域も書かない)
        document["classes"][0]["name"] = "WARRIOR"
        document["classes"][0]["spell_type"] = "NONE"
        document["classes"][0]["realms"] = []
        ok, message = self.validate("ClassMagicDefinitions", document)
        self.assertTrue(ok, message)

    def test_nested_records_must_be_objects(self):
        fields = (
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0]),
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0, "overrides", 0]),
            ("SpellDefinitions", ["realms", 0, "books", 0]),
            ("SpellDefinitions", ["realms", 0, "books", 0, "spells", 0]),
        )
        for name, path in fields:
            for value in (1, [], None):
                with self.subTest(name=name, path=path, value=value):
                    self.check_value(name, path, value, False, "object")

    def test_reader_accepted_empty_arrays_and_strings(self):
        magic = copy.deepcopy(self.samples["ClassMagicDefinitions"])
        magic["classes"][0]["realms"][0]["overrides"] = []
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
        # プリーストは選べる領域をすべて書くので、空の一覧は通らない
        self.check_value("ClassMagicDefinitions", ["classes", 0, "realms"], [], False, "realm LIFE that PRIEST can choose must be listed")
        self.check_value("SpellDefinitions", ["realms", 0, "books"], [], True)
        self.check_value("SpellDefinitions", ["realms", 0, "books", 0, "spells"], [], True)

    def test_version_policy_is_unchanged(self):
        # SpellDefinitions と ClassMagicDefinitions は reader が version を確かめる (test_*_version_must_be_integer_two)。
        for name in ("ClassSkillDefinitions",):
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
        magic["classes"][0]["realms"][0]["overrides"][0]["spell_tag"] = records[0]["spell_tag"]
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
                magic["classes"][0]["realms"][0]["overrides"][0]["spell_tag"] = ""
                ok, message = self.validate("SpellDefinitions", spells)
                self.assertTrue(ok, message)
                ok, message = self.validate("ClassMagicDefinitions", magic, spells)
                self.assertFalse(ok)
                self.assertIn("unknown spell tag in this realm", message)
                self.assertIn("Location: ['classes', 0, 'realms', 0, 'overrides', 0, 'spell_tag']", message)

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
                magic["classes"][0]["realms"][0]["overrides"][0]["spell_tag"] = ""
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
        magic["classes"][0]["realms"][0]["overrides"][0]["spell_tag"] = ""
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
            ("ClassMagicDefinitions", ["classes", 0, "realms", 0, "overrides", 0], "aptitude"),
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
