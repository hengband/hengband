"""DungeonReaderの数値契約を公開JSONC検証入口で確認する。"""
import copy
from pathlib import Path
import unittest

from json_validation_test_helper import validate_document
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
                (["final_floor", "guardian"], 1, 9999),
                (["final_floor", "object"], 1, 32767),
                (["final_floor", "artifact"], 0, 9999))

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
                      "towns": [{"id": 1, "name": {"ja": "Town", "en": "Town"}}],
                      "maps": {name: {"letters": [{"symbol": "#", "terrain": 0}],
                                      "layout": ["####", "####", "####"],
                                      "starting_position": {"y": 1, "x": 1}}
                               for name in ("normal", "compact")}}
        for y, x, accepted in ((0, 0, True), (2, 3, True), (3, 0, False), (0, 4, False)):
            with self.subTest(y=y, x=x):
                document = self.sample()
                document["dungeons"][0]["position"].update(wild_y=y, wild_x=x)
                ok, diagnostic = self.validate(document, companions={"WildernessDefinition.jsonc": wilderness})
                self.assertEqual(ok, accepted, diagnostic)
                if not accepted:
                    field = "wild_y" if y == 3 else "wild_x"
                    self.assertIn("dungeon entrance must be within the wilderness map", diagnostic)
                    self.assertIn(f"Location: {['dungeons', 0, 'position', field]}", diagnostic)
        wilderness["height"] = 3.0
        ok, diagnostic = self.validate(self.sample(), companions={"WildernessDefinition.jsonc": wilderness})
        self.assertFalse(ok, diagnostic)
        self.assertIn("Invalid wilderness definitions", diagnostic)

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
