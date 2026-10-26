"""JSONCのCI検証入口でモンスター種族の読込規則を確認する。"""
import copy
import re
from pathlib import Path
import unittest

from json_validation_test_helper import fixed_artifact_id_schema_maximum, monrace_id_schema_maximum, validate_document
from validate_json import build_schema_registry, load_all_schemas, load_jsonc


class MonraceValidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[2]
        cls.loaded, schema_map = load_all_schemas(cls.root / "schema")
        cls.schema_path = schema_map["MonraceDefinitions"]
        cls.registry = build_schema_registry(cls.loaded)
        cls.document = load_jsonc(cls.root / "lib/edit/MonraceDefinitions.jsonc")
        artifact_document = load_jsonc(cls.root / "lib/edit/ArtifactDefinitions.jsonc")
        artifacts = artifact_document["artifacts"]
        cls.artifact_version = artifact_document["version"]
        cls.max_artifact_id = max(artifact["id"] for artifact in artifacts)
        cls.artifact_template = copy.deepcopy(artifacts[0])
        cls.artifacts = artifacts

    def validate(self, monsters, *, companions=None):
        document = {"versions": self.document["versions"], "monsters": monsters}
        return validate_document(document, self.schema_path, self.loaded[self.schema_path], self.registry, companions=companions)


    def test_malformed_artifact_definitions_are_reported(self):
        # 参照先の欠落・型不正を例外のままにせず、対象ファイルの診断として返す。
        for companion in ([], {}, {"artifacts": None}, {"artifacts": [{}]}):
            with self.subTest(companion=companion):
                ok, diagnostic = self.validate([], companions={"ArtifactDefinitions.jsonc": companion})
                self.assertFalse(ok, diagnostic)
                self.assertIn("Invalid artifact definitions", diagnostic)
                self.assertIn("ArtifactDefinitions.jsonc", diagnostic)

    def test_adjacent_artifact_definitions_take_precedence(self):
        # 同梱表にないIDも隣接候補に定義されていれば参照でき、隣接候補に無ければ同梱表にあっても拒否する。
        # アーティファクトのIDは 1 から欠番なく並ぶため、隣接候補は同梱表の続きか先頭の一部にする。
        artifact = dict(self.artifact_template, id=self.max_artifact_id + 1)
        companion = {"version": self.artifact_version, "artifacts": self.artifacts + [artifact]}
        drop = {"drop_artifact_id": artifact["id"], "drop_probability": 50}
        ok, diagnostic = self.validate([self.monster(artifacts=[drop])],
                                       companions={"ArtifactDefinitions.jsonc": companion})
        self.assertTrue(ok, diagnostic)
        companion = {"version": self.artifact_version, "artifacts": self.artifacts[:2]}
        drop = dict(drop, drop_artifact_id=self.max_artifact_id)
        ok, diagnostic = self.validate([self.monster(artifacts=[drop])],
                                       companions={"ArtifactDefinitions.jsonc": companion})
        self.assertFalse(ok, diagnostic)
        self.assertIn("unknown artifact ID", diagnostic)

    def monster(self, monster_id=1, **fields):
        monster = copy.deepcopy(self.document["monsters"][1])
        for key in ("evolve", "artifacts", "escorts", "final_summon", "message"):
            monster.pop(key, None)
        monster["id"] = monster_id
        monster.update(fields)
        return monster

    def assert_valid(self, monsters):
        ok, diagnostic = self.validate(monsters)
        self.assertTrue(ok, diagnostic)

    def assert_invalid(self, monsters, path, reason=None):
        ok, diagnostic = self.validate(monsters)
        self.assertFalse(ok, diagnostic)
        self.assertIn(f"Location: {path}", diagnostic)
        if reason:
            self.assertIn(reason, diagnostic)

    def test_bundled_monsters(self):
        self.assert_valid(self.document["monsters"])

    def test_level_and_rarity_boundaries(self):
        self.assert_valid([self.monster(level=0, rarity=1), self.monster(2, level=127, rarity=255)])
        self.assert_invalid([self.monster(level=128)], ["monsters", 0, "level"])
        self.assert_invalid([self.monster(rarity=0)], ["monsters", 0, "rarity"])

    def test_armor_class_fits_range_whose_computations_do_not_overflow(self):
        # 五右衛門の AC を3倍しても16ビットに収まる範囲 (根拠は race-reader.cpp の MONRACE_AC_MAX)。
        self.assert_valid([self.monster(armor_class=0), self.monster(2, armor_class=10000)])
        self.assert_invalid([self.monster(armor_class=10001)], ["monsters", 0, "armor_class"])
        self.assert_invalid([self.monster(armor_class=-1)], ["monsters", 0, "armor_class"])
        source = (Path(__file__).resolve().parents[2] / "src/info-reader/race-reader.cpp").read_text(encoding="utf-8")
        match = re.search(r"constexpr int MONRACE_AC_MAX = (\d+);", source)
        self.assertIsNotNone(match, "MONRACE_AC_MAX not found in src/info-reader/race-reader.cpp")
        limit = int(match.group(1))
        schema = self.loaded[self.schema_path]["properties"]["monsters"]["items"]["properties"]["armor_class"]
        self.assertEqual(schema["maximum"], limit)
        self.assertLessEqual(limit * 3, 32767)

    def test_arena_odds_correction_ratio_boundaries(self):
        self.assert_valid([self.monster(odds_correction_ratio=1),
                           self.monster(2, odds_correction_ratio=1000)])
        for ratio in (0, 1001):
            with self.subTest(ratio=ratio):
                self.assert_invalid([self.monster(odds_correction_ratio=ratio)],
                                    ["monsters", 0, "odds_correction_ratio"])

    def test_evolution_experience_boundaries(self):
        for need_exp in (1, 9999999):
            with self.subTest(need_exp=need_exp):
                self.assert_valid([self.monster(evolve={"need_exp": need_exp, "to": 2}),
                                   self.monster(2)])
        for need_exp in (0, 10000000):
            with self.subTest(need_exp=need_exp):
                self.assert_invalid([self.monster(evolve={"need_exp": need_exp, "to": 2}),
                                     self.monster(2)], ["monsters", 0, "evolve", "need_exp"])

    def test_legacy_range_constants_match_schema(self):
        source = (self.root / "src/info-reader/race-reader.cpp").read_text(encoding="utf-8")
        properties = self.loaded[self.schema_path]["properties"]["monsters"]["items"]["properties"]
        for constant, schema, bound in (
            ("MONRACE_ODDS_CORRECTION_RATIO_MAX", properties["odds_correction_ratio"], "maximum"),
            ("MONRACE_EVOLVE_NEED_EXP_MIN", properties["evolve"]["properties"]["need_exp"], "minimum"),
        ):
            with self.subTest(constant=constant):
                match = re.search(rf"constexpr int {constant} = (\d+);", source)
                self.assertIsNotNone(match, f"{constant} not found in race-reader.cpp")
                self.assertEqual(schema[bound], int(match.group(1)))
        self.assertEqual(properties["evolve"]["properties"]["need_exp"]["maximum"],
                         properties["exp"]["maximum"])

    def test_float_integers_are_rejected(self):
        for field in ("speed", "level", "rarity", "exp"):
            with self.subTest(field=field):
                monster = self.monster()
                monster[field] = float(monster[field])
                self.assert_invalid([monster], ["monsters", 0, field], "integer JSON value")

    def test_monster_id_maximums_match_monrace_id_storage(self):
        # モンスターIDの上限は、IDを格納する型(MonraceId)の上限−1とする。
        maximum = monrace_id_schema_maximum()
        properties = self.loaded[self.schema_path]["properties"]["monsters"]["items"]["properties"]
        self.assertEqual(properties["id"]["maximum"], maximum)
        self.assertEqual(properties["evolve"]["properties"]["to"]["maximum"], maximum)
        self.assertEqual(properties["escorts"]["items"]["properties"]["escorts_id"]["maximum"], maximum)
        self.assertEqual(properties["final_summon"]["items"]["properties"]["id"]["maximum"], maximum)

    def test_monster_id_boundaries(self):
        self.assert_valid([self.monster(1), self.monster(32766)])
        self.assert_invalid([self.monster(1), self.monster(32767)], ["monsters", 1, "id"])
        cases = (
            (lambda target: {"evolve": {"need_exp": 10, "to": target}}, ["evolve", "to"]),
            (lambda target: {"escorts": [{"escorts_id": target, "escort_num": "1d2"}]}, ["escorts", 0, "escorts_id"]),
            (lambda target: {"final_summon": [{"id": target, "probability": 100, "min_num": 1, "max_num": 1, "radius": 1}]},
             ["final_summon", 0, "id"]),
        )
        for make, suffix in cases:
            with self.subTest(reference=suffix[0]):
                self.assert_valid([self.monster(**make(32766)), self.monster(32766)])
                self.assert_invalid([self.monster(**make(32767))], ["monsters", 0] + suffix)

    def test_drop_artifact_id_maximum_matches_fixed_artifact_id_storage(self):
        # 固定アーティファクトIDの上限は、IDを格納する型(FixedArtifactId)の上限−1とする。
        maximum = fixed_artifact_id_schema_maximum()
        properties = self.loaded[self.schema_path]["properties"]["monsters"]["items"]["properties"]
        self.assertEqual(properties["artifacts"]["items"]["properties"]["drop_artifact_id"]["maximum"], maximum)
        drop = {"drop_artifact_id": maximum + 1, "drop_probability": 50}
        self.assert_invalid([self.monster(artifacts=[drop])], ["monsters", 0, "artifacts", 0, "drop_artifact_id"])

    def test_ids_must_be_strictly_increasing(self):
        self.assert_valid([self.monster(1), self.monster(3)])
        for second_id in (1, 0):
            with self.subTest(second_id=second_id):
                self.assert_invalid([self.monster(1), self.monster(second_id)], ["monsters", 1, "id"], "strictly increasing")

    def test_dice_boundaries(self):
        cases = (
            ("hit_point", lambda dice: {"hit_point": dice}, ["hit_point"]),
            ("escort_num", lambda dice: {"escorts": [{"escorts_id": 2, "escort_num": dice}]}, ["escorts", 0, "escort_num"]),
            ("damage_dice", lambda dice: {"blows": [{"method": "HIT", "effect": "HURT", "damage_dice": dice}]}, ["blows", 0, "damage_dice"]),
            ("shoot", lambda dice: {"skill": {"probability": "1_IN_2", "shoot": dice}}, ["skill", "shoot"]),
        )
        for name, make, suffix in cases:
            with self.subTest(dice=name):
                self.assert_valid([self.monster(**make("1d32767")), self.monster(2), self.monster(3, **make("181d181"))])
                self.assert_invalid([self.monster(**make("1d32768")), self.monster(2)], ["monsters", 0] + suffix, "maximum roll")
                for dice in ("-1d5", "0d5", "1d0", "x1d5", "1d5x", "01d5", "1d05", "1d5\n"):
                    with self.subTest(value=dice):
                        self.assert_invalid([self.monster(**make(dice)), self.monster(2)], ["monsters", 0] + suffix)

    def test_skill_probability_rejects_trailing_newline(self):
        self.assert_valid([self.monster(skill={"probability": "1_IN_100", "shoot": "1d1"})])
        self.assert_invalid([self.monster(skill={"probability": "1_IN_2\n", "shoot": "1d1"})],
                            ["monsters", 0, "skill", "probability"])

    def test_symbol_must_be_one_character(self):
        for character in ("ab", "a\n"):
            with self.subTest(character=character):
                monster = self.monster()
                monster["symbol"]["character"] = character
                self.assert_invalid([monster], ["monsters", 0, "symbol", "character"])

    def test_monster_references_must_be_defined(self):
        later = self.monster(5)
        cases = (
            ("evolve", lambda target: {"evolve": {"need_exp": 10, "to": target}}, ["evolve", "to"]),
            ("escort", lambda target: {"escorts": [{"escorts_id": target, "escort_num": "1d2"}]}, ["escorts", 0, "escorts_id"]),
            ("final_summon", lambda target: {"final_summon": [{"id": target, "probability": 100, "min_num": 1, "max_num": 1, "radius": 1}]},
             ["final_summon", 0, "id"]),
        )
        for name, make, suffix in cases:
            with self.subTest(reference=name):
                # 後ろのIDも参照できる。
                self.assert_valid([self.monster(**make(5)), later])
                self.assert_invalid([self.monster(**make(4)), later], ["monsters", 0] + suffix, "unknown monster ID")

    def test_player_id_cannot_be_referenced(self):
        player = self.monster(0)
        self.assert_invalid([player, self.monster(1, escorts=[{"escorts_id": 0, "escort_num": "1d2"}])],
                            ["monsters", 1, "escorts", 0, "escorts_id"])

    def test_artifact_references_must_be_defined(self):
        drop = {"drop_artifact_id": self.max_artifact_id, "drop_probability": 50}
        self.assert_valid([self.monster(artifacts=[drop])])
        drop = dict(drop, drop_artifact_id=self.max_artifact_id + 1)
        self.assert_invalid([self.monster(artifacts=[drop])], ["monsters", 0, "artifacts", 0, "drop_artifact_id"], "unknown artifact ID")

    def test_final_summon_count_order(self):
        summon = {"id": 1, "probability": 100, "min_num": 2, "max_num": 2, "radius": 1}
        self.assert_valid([self.monster(final_summon=[summon])])
        summon = dict(summon, min_num=3)
        self.assert_invalid([self.monster(final_summon=[summon])], ["monsters", 0, "final_summon", 0, "min_num"], "must not exceed")

    def test_start_hp_percentage_requires_positive_value(self):
        self.assert_valid([self.monster(start_hp_percentage=1)])
        self.assert_invalid([self.monster(start_hp_percentage=0)], ["monsters", 0, "start_hp_percentage"])


if __name__ == "__main__":
    unittest.main()
