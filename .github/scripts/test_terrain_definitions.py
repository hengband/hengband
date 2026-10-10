"""JSONCのCI検証入口で地形定義の数値読込規則を確認する。"""
import copy
from pathlib import Path
import unittest

from json_validation_test_helper import validate_document
from validate_json import build_schema_registry, load_all_schemas, load_jsonc


class TerrainValidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[2]
        cls.loaded, schema_map = load_all_schemas(cls.root / "schema")
        cls.schema_path = schema_map["TerrainDefinitions"]
        cls.schema = cls.loaded[cls.schema_path]
        cls.registry = build_schema_registry(cls.loaded)

    def terrain(self):
        return {"id": 0, "key": "FLOOR", "name": {"ja": "Floor", "en": "Floor"},
                "symbol": {"character": ".", "color": "w", "lit": False}, "flags": []}

    def generation(self, probabilities):
        return {"changes": [{"terrain": "FLOOR", "probability": value} for value in probabilities]}

    def numeric_cases(self):
        return ((["id"], 0, 9999), (["map_priority"], -9999, 9999),
                (["trap", "power"], 0, 255), (["door", "power"], 0, 255),
                (["tunnel", "power"], 0, 255), (["convert", "stream_index"], 0, 254),
                (["generation", "changes", 0, "probability"], 1, 100))

    def with_number(self, path, value):
        terrain = self.terrain()
        terrain.update({"trap": {"type": "PIT", "power": 0}, "door": {"power": 0},
                        "tunnel": {"power": 0}, "convert": {"type": "STREAM", "stream_index": 0},
                        "generation": self.generation([1])})
        target = terrain
        for part in path[:-1]:
            target = target[part]
        target[path[-1]] = value
        return terrain

    def validate(self, terrains):
        return validate_document({"version": 1, "terrains": terrains}, self.schema_path, self.schema, self.registry)

    def assert_valid(self, terrains):
        ok, diagnostic = self.validate(terrains)
        self.assertTrue(ok, diagnostic)

    def assert_invalid(self, terrains, path, reason=None):
        ok, diagnostic = self.validate(terrains)
        self.assertFalse(ok, diagnostic)
        self.assertIn(f"Location: {path}", diagnostic)
        if reason:
            self.assertIn(reason, diagnostic)

    def test_bundled_terrains(self):
        document = load_jsonc(self.root / "lib/edit/TerrainDefinitions.jsonc")
        self.assert_valid(document["terrains"])

    def test_integer_boundaries(self):
        for path, minimum, maximum in self.numeric_cases():
            for value in (minimum, maximum):
                with self.subTest(path=path, value=value):
                    self.assert_valid([self.with_number(path, value)])

    def test_out_of_range_numbers(self):
        for path, minimum, maximum in self.numeric_cases():
            for value in (minimum - 1, maximum + 1):
                with self.subTest(path=path, value=value):
                    self.assert_invalid([self.with_number(path, value)], ["terrains", 0] + path)

    def test_integral_floats_pass_schema_but_fail_reader_checks(self):
        for path, minimum, maximum in self.numeric_cases():
            for value in (float(minimum), float(maximum)):
                with self.subTest(path=path, value=value):
                    terrain = self.with_number(path, value)
                    self.assert_invalid([terrain], ["terrains", 0] + path, "integer JSON value")

    def test_invalid_number_types(self):
        for path, _, _ in self.numeric_cases():
            for value in (True, False, "1", [], {}, 1.5):
                with self.subTest(path=path, value=value):
                    self.assert_invalid([self.with_number(path, value)], ["terrains", 0] + path)

    def test_required_numbers_cannot_be_null(self):
        for path, _, _ in self.numeric_cases():
            if path == ["map_priority"]:
                continue
            with self.subTest(path=path):
                self.assert_invalid([self.with_number(path, None)], ["terrains", 0] + path)

    def test_optional_values_may_be_absent_or_null(self):
        terrain = self.terrain()
        self.assert_valid([terrain])
        for field in ("map_priority", "trap", "door", "tunnel", "convert", "generation"):
            terrain[field] = None
        self.assert_valid([terrain])

    def test_stream_requires_index(self):
        terrain = self.terrain()
        terrain["convert"] = {"type": "STREAM"}
        self.assert_invalid([terrain], ["terrains", 0, "convert"], "required property")

    def test_non_stream_index_uses_same_bounds_when_present(self):
        for conversion_type in ("FLOOR", "WALL", "INNER", "OUTER", "SOLID"):
            terrain = self.terrain()
            terrain["convert"] = {"type": conversion_type}
            self.assert_valid([terrain])
            for index in (0, 254):
                with self.subTest(conversion_type=conversion_type, index=index):
                    terrain["convert"]["stream_index"] = index
                    self.assert_valid([terrain])
            for index in (-1, 255, 2**64, 1.0):
                with self.subTest(conversion_type=conversion_type, index=index):
                    terrain["convert"]["stream_index"] = index
                    self.assert_invalid([terrain], ["terrains", 0, "convert", "stream_index"])

    def test_empty_key_is_rejected(self):
        terrain = self.terrain()
        terrain["key"] = ""
        self.assert_invalid([terrain], ["terrains", 0, "key"])

    def test_generation_sum_at_or_below_limit(self):
        for probabilities in ([], [1], [100], [50, 50], [1] * 100):
            with self.subTest(probabilities=probabilities):
                terrain = self.terrain()
                terrain["generation"] = self.generation(probabilities)
                self.assert_valid([terrain])

    def test_generation_sum_over_limit(self):
        for probabilities, failing_index in (([50, 51], 1), ([50, 40, 11], 2), ([1] * 101, 100)):
            with self.subTest(probabilities=probabilities):
                terrain = self.terrain()
                terrain["generation"] = self.generation(probabilities)
                self.assert_invalid([terrain], ["terrains", 0, "generation", "changes", failing_index, "probability"],
                                    "sum must not exceed 100")

    def test_generation_sum_resets_for_each_terrain(self):
        first = self.terrain()
        first["generation"] = self.generation([100])
        second = copy.deepcopy(first)
        second["id"] = 1
        self.assert_valid([first, second])

    def test_later_terrain_error_location(self):
        second = self.with_number(["door", "power"], 1.0)
        self.assert_invalid([self.terrain(), second], ["terrains", 1, "door", "power"], "integer JSON value")

    def test_duplicate_and_descending_ids_are_allowed(self):
        terrains = [self.terrain() for _ in range(3)]
        for terrain, terrain_id in zip(terrains, (2, 2, 0)):
            terrain["id"] = terrain_id
        self.assert_valid(terrains)

    def test_empty_terrains_are_allowed(self):
        self.assert_valid([])

    def test_version_is_not_a_reader_integer_field(self):
        ok, diagnostic = validate_document({"version": 1.5, "terrains": [self.terrain()]},
                                           self.schema_path, self.schema, self.registry)
        self.assertTrue(ok, diagnostic)


if __name__ == "__main__":
    unittest.main()
