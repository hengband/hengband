"""DungeonReaderの数値契約を公開JSONC検証入口で確認する。"""
import copy
import re
from pathlib import Path
import unittest

from json_validation_test_helper import fixed_artifact_id_schema_maximum, monrace_id_schema_maximum, validate_document
from validate_json import build_schema_registry, load_all_schemas, load_cpp_map_keys, load_jsonc


class DungeonValidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[2]
        cls.loaded, cls.mapping = load_all_schemas(cls.root / "schema")
        cls.schema_path = cls.mapping["DungeonDefinitions"]
        cls.schema = cls.loaded[cls.schema_path]
        cls.registry = build_schema_registry(cls.loaded)
        cls.bundled = load_jsonc(cls.root / "lib/edit/DungeonDefinitions.jsonc")

    def sample(self):
        dungeon = copy.deepcopy(self.bundled["dungeons"][1])
        dungeon["final_floor"] = {"guardian": 237, "object": 148, "artifact": 89}
        for group, terrain in (("floor", "FLOOR"), ("wall", "GRANITE")):
            dungeon[group]["tiles"] = [{"type": terrain, "rate": 50}, {"type": terrain, "rate": 50}]
        return {"version": 1, "dungeons": [dungeon]}

    def validate(self, document, *, companions=None):
        return validate_document(document, self.schema_path, self.schema, self.registry, companions=companions)

    def reader_token_tables(self, function):
        """DungeonReaderの指定関数が照合する r_info_* の辞書名を、照合の順に読む。"""
        source = (self.root / "src/info-reader/dungeon-reader.cpp").read_text(encoding="utf-8")
        match = re.search(r"\bDungeonReader::" + re.escape(function) + r"\(.*?\n\{(.*?)^\}", source, re.DOTALL | re.MULTILINE)
        self.assertIsNotNone(match, f"{function} not found in dungeon-reader.cpp")
        tables = re.findall(r"grab_one_flag\(\s*dungeon\.\w+\s*,\s*(r_info_\w+)\s*,", match[1])
        self.assertTrue(tables, f"no token tables found in {function}")
        return tables

    def reader_tokens(self, function):
        path = self.root / "src/info-reader/race-info-tokens-table.cpp"
        tokens = set()
        for table in self.reader_token_tables(function):
            keys = load_cpp_map_keys(path, table)
            # NONEは種族の性別未設定を表し、DungeonReaderも条件フラグとしては拒否する。
            tokens.update(set(keys) - {"NONE"} if table == "r_info_sex" else keys)
        return tokens

    def test_monster_flags_match_the_reader_token_tables(self):
        # flags には grab_one_basic_monster_flag が照合する辞書のトークンをすべて書け、それ以外は書けない。
        self.assertEqual(set(self.schema["$defs"]["monsterFlag"]["enum"]), self.reader_tokens("grab_one_basic_monster_flag"))

    def test_monster_spells_match_the_reader_token_tables(self):
        # spells には grab_one_spell_monster_flag が照合する辞書のトークンをすべて書け、それ以外は書けない。
        self.assertEqual(set(self.schema["$defs"]["monsterSpell"]["enum"]), self.reader_tokens("grab_one_spell_monster_flag"))

    def test_every_reader_token_is_accepted(self):
        for field, function in (("flags", "grab_one_basic_monster_flag"), ("spells", "grab_one_spell_monster_flag")):
            with self.subTest(field=field):
                document = self.sample()
                document["dungeons"][0]["monsters"][field] = sorted(self.reader_tokens(function))
                ok, diagnostic = self.validate(document)
                self.assertTrue(ok, diagnostic)

    def test_tokens_of_tables_the_reader_does_not_consult_are_rejected(self):
        # 発言の辞書 (r_info_message_flags) だけにある WALK_* などや、flags と spells の取り違えは読み込みが拒否する。
        cases = (("flags", "WALK_CLOSERANGE"), ("flags", "MESSAGE_STALKER"), ("flags", "BR_FIRE"), ("spells", "UNIQUE"))
        for field, token in cases:
            with self.subTest(field=field, token=token):
                document = self.sample()
                document["dungeons"][0]["monsters"][field] = [token]
                ok, diagnostic = self.validate(document)
                self.assertFalse(ok, diagnostic)
                self.assertIn(f"Location: ['dungeons', 0, 'monsters', '{field}', 0]", diagnostic)

    def test_monster_sex_flags_are_accepted(self):
        self.assertIn("r_info_sex", self.reader_token_tables("grab_one_basic_monster_flag"))
        for flags in (["MALE"], ["FEMALE"], ["MALE", "FEMALE"]):
            with self.subTest(flags=flags):
                document = self.sample()
                document["dungeons"][0]["monsters"]["flags"] = flags
                ok, diagnostic = self.validate(document)
                self.assertTrue(ok, diagnostic)

    def test_unknown_sex_flags_are_rejected(self):
        for token in ("NONE", "SEX_UNKNOWN"):
            with self.subTest(token=token):
                document = self.sample()
                document["dungeons"][0]["monsters"]["flags"] = [token]
                ok, diagnostic = self.validate(document)
                self.assertFalse(ok, diagnostic)
                self.assertIn("Location: ['dungeons', 0, 'monsters', 'flags', 0]", diagnostic)

    def test_unknown_aura_flags_are_rejected(self):
        document = self.sample()
        document["dungeons"][0]["monsters"]["flags"] = ["AURA_FIRE", "AURA_UNKNOWN"]
        ok, diagnostic = self.validate(document)
        self.assertFalse(ok, diagnostic)
        self.assertIn("Location: ['dungeons', 0, 'monsters', 'flags', 1]", diagnostic)

    def numeric_cases(self):
        return ((["id"], 0, 20),
                (["position", "wild_y"], 0, 32766),
                (["position", "wild_x"], 0, 32766),
                (["generation", "minDepth"], 0, 32767),
                (["generation", "maxDepth"], 0, 32767),
                (["generation", "minPlayerLevel"], 0, 50),
                (["generation", "objGood"], 0, 100),
                (["generation", "objGreat"], 0, 100),
                (["monsters", "minCount"], 0, 100),
                (["monsters", "additionalSpawnProbability"], 1, 1000000),
                (["monsters", "normalMonsterRate"], 0, 100),
                (["floor", "tunnelRate"], 0, 32767),
                (["floor", "tiles", 0, "rate"], 0, 100),
                (["wall", "tiles", 0, "rate"], 0, 100),
                (["wall", "streams", 0, "count"], 1, 255),
                (["wall", "streams", 0, "chance"], 1, 65535),
                (["wall", "streams", 0, "priority"], 0, 255),
                (["final_floor", "guardian"], 1, 32766),
                (["final_floor", "object"], 1, 32767),
                (["final_floor", "artifact"], 0, 32766))

    def with_number(self, path, value):
        document = self.sample()
        target = document["dungeons"][0]
        for part in path[:-1]:
            target = target[part]
        target[path[-1]] = value
        # 深さの単独境界試験では対になる階層も揃え、大小関係の誤りを混ぜない。
        generation = document["dungeons"][0]["generation"]
        if path == ["generation", "minDepth"] and type(value) in (int, float):
            generation["maxDepth"] = max(0, min(32767, int(value)))
        elif path == ["generation", "maxDepth"]:
            generation["minDepth"] = 0
        # 地形の重みの単独境界試験では、もう一方の重みで合計を100に揃え、合計の誤りを混ぜない。
        if path[1:] == ["tiles", 0, "rate"] and type(value) is int and 0 <= value <= 100:
            document["dungeons"][0][path[0]]["tiles"][1]["rate"] = 100 - value
        # ID0は入口配置を行わない。座標の保存上限はこのレコードで独立して検証する。
        if path[0] == "position":
            document["dungeons"][0]["id"] = 0
        return document

    def test_bundled_document_is_accepted(self):
        ok, diagnostic = self.validate(self.bundled)
        self.assertTrue(ok, diagnostic)

    def test_storage_and_existing_reader_range_boundaries(self):
        for path, minimum, maximum in self.numeric_cases():
            for number in (minimum, maximum):
                with self.subTest(path=path, number=number):
                    ok, diagnostic = self.validate(self.with_number(path, number))
                    self.assertTrue(ok, diagnostic)

    def test_final_floor_guardian_maximum_matches_monrace_id_storage(self):
        # モンスターIDの上限は、IDを格納する型(MonraceId)の上限−1とする。
        maximum = monrace_id_schema_maximum()
        self.assertEqual(self.schema["$defs"]["final_floor"]["properties"]["guardian"]["maximum"], maximum)

    def test_final_floor_artifact_maximum_matches_fixed_artifact_id_storage(self):
        # 固定アーティファクトIDの上限は、IDを格納する型(FixedArtifactId)の上限−1とする。
        maximum = fixed_artifact_id_schema_maximum()
        self.assertEqual(self.schema["$defs"]["final_floor"]["properties"]["artifact"]["maximum"], maximum)

    def test_values_outside_reader_ranges_are_rejected(self):
        for path, minimum, maximum in self.numeric_cases():
            for number in (minimum - 1, maximum + 1):
                with self.subTest(path=path, number=number):
                    ok, diagnostic = self.validate(self.with_number(path, number))
                    self.assertFalse(ok, diagnostic)
                    self.assertIn(f"Location: {['dungeons', 0] + path}", diagnostic)

    def test_integral_floats_rejected_with_field_location(self):
        for path, minimum, maximum in self.numeric_cases():
            for number in (float(minimum), float(maximum)):
                with self.subTest(path=path, number=number):
                    ok, diagnostic = self.validate(self.with_number(path, number))
                    self.assertFalse(ok, diagnostic)
                    self.assertIn("integer JSON value", diagnostic)
                    self.assertIn(f"Location: {['dungeons', 0] + path}", diagnostic)

    def test_other_invalid_number_types_are_rejected(self):
        for path, _, _ in self.numeric_cases():
            for value in (True, False, "1", 1.5, None):
                with self.subTest(path=path, value=value):
                    ok, diagnostic = self.validate(self.with_number(path, value))
                    self.assertFalse(ok, diagnostic)

    def test_duplicate_or_descending_ids_are_rejected(self):
        for first, second in ((2, 2), (2, 1)):
            document = self.sample()
            document["dungeons"][0]["id"] = first
            later = copy.deepcopy(document["dungeons"][0])
            later["id"] = second
            document["dungeons"].append(later)
            ok, diagnostic = self.validate(document)
            self.assertFalse(ok, diagnostic)
            self.assertIn("strictly increasing", diagnostic)
            self.assertIn("Location: ['dungeons', 1, 'id']", diagnostic)

    def test_gaps_in_ids_remain_valid(self):
        document = self.sample()
        document["dungeons"][0]["id"] = 0
        later = copy.deepcopy(document["dungeons"][0])
        later["id"] = 20
        document["dungeons"].append(later)
        ok, diagnostic = self.validate(document)
        self.assertTrue(ok, diagnostic)

    def test_bundled_tile_rates_are_percentages(self):
        for dungeon in self.bundled["dungeons"]:
            for group in ("floor", "wall"):
                with self.subTest(id=dungeon["id"], group=group):
                    rates = [tile["rate"] for tile in dungeon[group]["tiles"]]
                    self.assertTrue(all(0 <= rate <= 100 for rate in rates), rates)
                    self.assertEqual(sum(rates), 100)

    def test_schema_tile_rates_are_bounded_as_percentages(self):
        for definition in ("floorTile", "wallTile"):
            with self.subTest(definition=definition):
                rate = self.schema["$defs"][definition]["properties"]["rate"]
                self.assertEqual(rate["minimum"], 0)
                self.assertEqual(rate["maximum"], 100)

    def test_tile_rates_totaling_100_are_accepted(self):
        for group in ("floor", "wall"):
            for rates in ([100], [0, 100], [100, 0], [1, 99], [20, 30, 50]):
                with self.subTest(group=group, rates=rates):
                    document = self.sample()
                    terrain = document["dungeons"][0][group]["tiles"][0]["type"]
                    document["dungeons"][0][group]["tiles"] = [{"type": terrain, "rate": rate} for rate in rates]
                    ok, diagnostic = self.validate(document)
                    self.assertTrue(ok, diagnostic)

    def test_tile_rate_totals_other_than_100_are_rejected(self):
        for group in ("floor", "wall"):
            for rates in ([0], [0, 0], [99], [49, 50], [50, 51], [100, 1], [100, 100]):
                with self.subTest(group=group, rates=rates):
                    document = self.sample()
                    terrain = document["dungeons"][0][group]["tiles"][0]["type"]
                    document["dungeons"][0][group]["tiles"] = [{"type": terrain, "rate": rate} for rate in rates]
                    ok, diagnostic = self.validate(document)
                    self.assertFalse(ok, diagnostic)
                    self.assertIn(f"must total exactly 100 (got {sum(rates)})", diagnostic)
                    self.assertIn(f"Location: {['dungeons', 0, group, 'tiles']}", diagnostic)

    def test_entrances_follow_candidate_wilderness_dimensions(self):
        wilderness = {"version": 1, "width": 4, "height": 3,
                      "towns": [{"id": 1, "name": {"ja": "Town", "en": "Town"}, "alias": {"ja": "Town", "en": "Town"}}]
                      + [{"id": town_id, "name": {"ja": "Town", "en": "Town"}} for town_id in range(2, 6)],
                      "maps": {name: {"letters": [{"symbol": "#", "terrain": 0}],
                                      "layout": ["####", "####", "####"],
                                      "starting_position": {"y": 1, "x": 1}}
                               for name in ("normal", "compact")}}
        # 3×4の荒野で端を除いた内側は y=1・x=1〜2 だけ。
        cases = ((1, 1, None), (1, 2, None), (0, 1, "wild_y"), (2, 1, "wild_y"), (3, 1, "wild_y"),
                 (1, 0, "wild_x"), (1, 3, "wild_x"), (1, 4, "wild_x"))
        for y, x, rejected_field in cases:
            with self.subTest(y=y, x=x):
                document = self.sample()
                document["dungeons"][0]["position"].update(wild_y=y, wild_x=x)
                ok, diagnostic = self.validate(document, companions={"WildernessDefinition.jsonc": wilderness})
                self.assertEqual(ok, rejected_field is None, diagnostic)
                if rejected_field:
                    # 0 は schema が拒否し、荒野の大きさを超える位置は意味の検査が拒否する。
                    if min(y, x) >= 1:
                        self.assertIn("dungeon entrance must be inside the wilderness map excluding the edges", diagnostic)
                    self.assertIn(f"{['dungeons', 0, 'position', rejected_field]}", diagnostic)
        wilderness["height"] = 3.0
        ok, diagnostic = self.validate(self.sample(), companions={"WildernessDefinition.jsonc": wilderness})
        self.assertFalse(ok, diagnostic)
        self.assertIn("Invalid wilderness definitions", diagnostic)

    def test_entrance_limits_match_largest_wilderness(self):
        # 荒野 (ID 0) 以外の入口は荒野の端を除いた内側に限られ、荒野は最大で MAX_WID×MAX_HGT になる。
        header = (self.root / "src/floor/floor-base-definitions.h").read_text(encoding="utf-8")
        limits = {name: int(value) for name, value in re.findall(r"constexpr int (MAX_HGT|MAX_WID) = (\d+);", header)}
        self.assertEqual(set(limits), {"MAX_HGT", "MAX_WID"}, "MAX_HGT / MAX_WID not found in floor-base-definitions.h")
        position = self.schema["$defs"]["entrancePosition"]["properties"]
        self.assertEqual((position["wild_y"]["minimum"], position["wild_y"]["maximum"]), (1, limits["MAX_HGT"] - 2))
        self.assertEqual((position["wild_x"]["minimum"], position["wild_x"]["maximum"]), (1, limits["MAX_WID"] - 2))

    def test_entrances_of_dungeons_fit_the_largest_wilderness(self):
        # 最大の荒野 (198×66) を隣に置き、荒野 (ID 0) 以外の入口の境界を確かめる。
        width, height = 198, 66
        wilderness = {"version": 1, "width": width, "height": height,
                      "towns": [{"id": 1, "name": {"ja": "Town", "en": "Town"}, "alias": {"ja": "Town", "en": "Town"}}]
                      + [{"id": town_id, "name": {"ja": "Town", "en": "Town"}} for town_id in range(2, 6)],
                      "maps": {name: {"letters": [{"symbol": "#", "terrain": 0}],
                                      "layout": ["#" * width] * height,
                                      "starting_position": {"y": 1, "x": 1}}
                               for name in ("normal", "compact")}}
        cases = ((1, 1, None), (height - 2, width - 2, None), (0, 1, "wild_y"), (height - 1, 1, "wild_y"),
                 (1, 0, "wild_x"), (1, width - 1, "wild_x"))
        for y, x, rejected_field in cases:
            with self.subTest(y=y, x=x):
                document = self.sample()
                document["dungeons"][0]["position"].update(wild_y=y, wild_x=x)
                ok, diagnostic = self.validate(document, companions={"WildernessDefinition.jsonc": wilderness})
                self.assertEqual(ok, rejected_field is None, diagnostic)
                if rejected_field:
                    self.assertIn(f"{['dungeons', 0, 'position', rejected_field]}", diagnostic)

    def test_artifact_sentinel_and_guardian_requirement(self):
        document = self.sample()
        document["dungeons"][0]["final_floor"]["artifact"] = 0
        ok, diagnostic = self.validate(document)
        self.assertTrue(ok, diagnostic)
        del document["dungeons"][0]["final_floor"]["guardian"]
        ok, diagnostic = self.validate(document)
        self.assertFalse(ok, diagnostic)
        self.assertIn("guardian", diagnostic)

    def test_inverted_depths_are_rejected(self):
        document = self.sample()
        document["dungeons"][0]["generation"].update(minDepth=100, maxDepth=99)
        ok, diagnostic = self.validate(document)
        self.assertFalse(ok, diagnostic)
        self.assertIn("minDepth must not exceed maxDepth", diagnostic)
        self.assertIn("Location: ['dungeons', 0, 'generation', 'minDepth']", diagnostic)

    def test_existing_deep_dungeon_and_tunnel_threshold_are_preserved(self):
        document = self.sample()
        document["dungeons"][0]["generation"].update(minDepth=666, maxDepth=696)
        document["dungeons"][0]["floor"]["tunnelRate"] = 1000
        ok, diagnostic = self.validate(document)
        self.assertTrue(ok, diagnostic)


if __name__ == "__main__":
    unittest.main()
