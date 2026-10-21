"""DungeonReaderの数値契約を公開JSONC検証入口で確認する。"""
import copy
import re
from pathlib import Path
import unittest

from json_validation_test_helper import fixed_artifact_id_schema_maximum, monrace_id_schema_maximum, validate_document
from validate_json import build_schema_registry, load_all_schemas, load_jsonc


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
            dungeon[group]["tiles"] = [{"type": terrain, "rate": 1}, {"type": terrain, "rate": 1}]
        return {"version": 1, "dungeons": [dungeon]}

    def validate(self, document, *, companions=None):
        return validate_document(document, self.schema_path, self.schema, self.registry, companions=companions)

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
                (["floor", "tiles", 0, "rate"], 0, 32767),
                (["wall", "tiles", 0, "rate"], 0, 32767),
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

    def test_zero_total_weights_are_rejected(self):
        for group in ("floor", "wall"):
            document = self.sample()
            for tile in document["dungeons"][0][group]["tiles"]:
                tile["rate"] = 0
            ok, diagnostic = self.validate(document)
            self.assertFalse(ok, diagnostic)
            self.assertIn("positive total weight", diagnostic)
            self.assertIn(f"Location: {['dungeons', 0, group, 'tiles']}", diagnostic)

    def test_weight_total_int_limit(self):
        # 個々のrateを32767以下に保ち、実際の公開入口で合計境界を検証する。
        # 2MB程度の文書2件を順次処理して、ピークメモリを抑える。
        count, remainder = divmod(2147483647, 32767)
        for tail in (remainder, remainder + 1):
            document = self.sample()
            tiles = [{"type": "FLOOR", "rate": 32767}] * count
            document["dungeons"][0]["floor"]["tiles"] = tiles + [{"type": "FLOOR", "rate": tail}]
            ok, diagnostic = self.validate(document)
            self.assertEqual(ok, tail == remainder, diagnostic)
            if tail != remainder:
                self.assertIn("total weight exceeds the reader's integer range", diagnostic)
                self.assertIn(f"Location: {['dungeons', 0, 'floor', 'tiles', count, 'rate']}", diagnostic)

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
