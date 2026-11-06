"""JSONCのCI検証入口で地形定義の数値・参照・表示・変化を確認する。"""
import copy
from pathlib import Path
import unittest
from jsonschema import ValidationError, validate

from json_validation_test_helper import terrain_id_schema_maximum, validate_document
from validate_json import build_schema_registry, load_all_schemas, load_jsonc, validate_terrain_semantics


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
                "symbol": {"character": ".", "color": "White", "lit": False}, "flags": []}

    def generation(self, probabilities):
        return {"changes": [{"terrain": "FLOOR", "probability": value} for value in probabilities]}

    def numeric_cases(self):
        return ((["id"], 0, 32766), (["map_priority"], 0, 125),
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

    def test_id_maximum_matches_feat_idx_storage(self):
        # 地形IDの上限は、IDを格納する型(FEAT_IDX)の上限−1とする。
        maximum = terrain_id_schema_maximum()
        self.assertEqual(self.schema["properties"]["terrains"]["items"]["properties"]["id"]["maximum"], maximum)

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
        second["key"] = "SECOND"
        self.assert_valid([first, second])

    def test_later_terrain_error_location(self):
        second = self.with_number(["door", "power"], 1.0)
        second["id"] = 1
        second["key"] = "SECOND"
        self.assert_invalid([self.terrain(), second], ["terrains", 1, "door", "power"], "integer JSON value")

    def test_descending_ids_are_allowed(self):
        terrains = [self.terrain() for _ in range(3)]
        for terrain, terrain_id, key in zip(terrains, (2, 1, 0), ("FIRST", "SECOND", "FLOOR")):
            terrain["id"] = terrain_id
            terrain["key"] = key
        self.assert_valid(terrains)

    def test_duplicate_ids_and_keys_are_rejected(self):
        second = self.terrain()
        second["key"] = "SECOND"
        self.assert_invalid([self.terrain(), second], ["terrains", 1, "id"], "unique")
        second["id"] = 1
        second["key"] = "FLOOR"
        self.assert_invalid([self.terrain(), second], ["terrains", 1, "key"], "unique")

    def test_symbol_requires_one_utf8_byte(self):
        for character in (".", " ", "\x00", "\x7f"):
            terrain = self.terrain()
            terrain["symbol"]["character"] = character
            self.assert_valid([terrain])
        for character in ("あ", "é", "😀", "..", ""):
            with self.subTest(character=character):
                terrain = self.terrain()
                terrain["symbol"]["character"] = character
                self.assert_invalid([terrain], ["terrains", 0, "symbol", "character"])

    def test_symbol_color_matches_reader_tokens(self):
        for color in ("Black", "White", "Dark Gray", "Light Brown"):
            terrain = self.terrain()
            terrain["symbol"]["color"] = color
            self.assert_valid([terrain])
        for color in ("w", "white", " White", "Unknown", ""):
            with self.subTest(color=color):
                terrain = self.terrain()
                terrain["symbol"]["color"] = color
                self.assert_invalid([terrain], ["terrains", 0, "symbol", "color"])

    def test_optional_mimic_and_interactions(self):
        for mimic in (None, "", "FLOOR"):
            for interactions in (None, {}):
                terrain = self.terrain()
                terrain.update(mimic=mimic, interactions=interactions)
                self.assert_valid([terrain])

    def test_tag_references_use_candidate_document_and_allow_forward_reference(self):
        first = self.terrain()
        first.update(mimic="SECOND", generation={"changes": [{"terrain": "SECOND", "probability": 1}]},
                     interactions={"OPEN": "SECOND", "DESTROYED": "SECOND"})
        second = self.terrain()
        second.update(id=1, key="SECOND", mimic="FLOOR")
        self.assert_valid([first, second])
        for field, path in (("mimic", ["mimic"]),
                            ("generation", ["generation", "changes", 0, "terrain"]),
                            ("interactions", ["interactions", "OPEN"]),
                            ("interactions", ["interactions", "DESTROYED"])):
            with self.subTest(path=path):
                terrain = self.terrain()
                if field == "mimic":
                    terrain[field] = "GRANITE"
                elif field == "generation":
                    terrain[field] = {"changes": [{"terrain": "GRANITE", "probability": 1}]}
                else:
                    terrain[field] = {path[-1]: "GRANITE"}
                # 同梱データにGRANITEがあっても候補文書には存在しない。
                self.assert_invalid([terrain], ["terrains", 0] + path, "undefined terrain tag")

    def test_interaction_tokens_and_eight_state_limit(self):
        terrain = self.terrain()
        actions = ("BASH", "CLOSE", "DISARM", "ENSECRET", "HIT_TRAP", "OPEN", "SECRET", "SPIKE")
        terrain["interactions"] = dict.fromkeys(actions, "FLOOR")
        terrain["interactions"].update(DESTROYED="FLOOR", DROP_GOLD="1d1")
        self.assert_valid([terrain])
        # JSON挿入順の末尾SPIKEではなく、readerのキー順の末尾UNPERMを診断する。
        terrain["interactions"] = {"UNPERM": "FLOOR", **terrain["interactions"]}
        self.assert_invalid([terrain], ["terrains", 0, "interactions", "UNPERM"], "eight")
        # 未知アクションでも、9件目ならreaderと同じくスロット枯渇を先に診断する。
        terrain["interactions"] = {"TYPO": "FLOOR", **dict.fromkeys(actions, "FLOOR")}
        self.assert_invalid([terrain], ["terrains", 0, "interactions", "TYPO"], "eight")
        terrain["interactions"]["TYPO"] = "MISSING"
        self.assert_invalid([terrain], ["terrains", 0, "interactions", "TYPO"], "eight")
        terrain["interactions"] = {"TYPO": "FLOOR"}
        self.assert_invalid([terrain], ["terrains", 0, "interactions", "TYPO"])
        terrain["interactions"] = {"OPEN": ""}
        self.assert_invalid([terrain], ["terrains", 0, "interactions", "OPEN"])

    def test_reader_flag_tokens_and_actions(self):
        terrain = self.terrain()
        terrain["flags"] = ["", "DOOR", "TUNNEL", "HAS_GOLD", "MAY_HAVE_GOLD", "CONVERT", "ENSECRET", "UNPERM"]
        terrain["interactions"] = {"MOVE": "FLOOR"}
        self.assert_valid([terrain])
        terrain["flags"] = ["TRAP_PIT"]
        self.assert_invalid([terrain], ["terrains", 0, "flags", 0])
        terrain["flags"] = []
        terrain["interactions"] = {"XXX00": "FLOOR"}
        self.assert_invalid([terrain], ["terrains", 0, "interactions", "XXX00"])

    def test_drop_gold_dice_syntax_and_int_limits(self):
        for dice in ("1d1", "001d002", "2147483647d1", "1d2147483647", "0" * 5000 + "1d2", "1d" + "0" * 5000 + "2"):
            terrain = self.terrain()
            terrain["interactions"] = {"DROP_GOLD": dice}
            self.assert_valid([terrain])
        for dice in ("2147483648d1", "1d2147483648", "9" * 5000 + "d1", "0d1", "1d0", "-1d2", "+1d2", "1d2junk", "1d2\n", "1d2\x00", "1D2", "1d2d3"):
            with self.subTest(dice=dice[:40]):
                terrain = self.terrain()
                terrain["interactions"] = {"DROP_GOLD": dice}
                self.assert_invalid([terrain], ["terrains", 0, "interactions", "DROP_GOLD"])

    def test_schema_token_spelling_keeps_cpp_vocabulary_validation_separate(self):
        terrain = self.terrain()
        terrain["flags"] = ["", "DOOR", "UNKNOWN_FLAG_1"]
        terrain["interactions"] = {"UNKNOWN_ACTION_1": "FLOOR", "DESTROYED": "FLOOR", "DROP_GOLD": "1d1"}
        validate({"version": 1, "terrains": [terrain]}, self.schema)
        for flag in ("door", "DOOR-", "DOOR\n"):
            with self.subTest(flag=flag):
                invalid = copy.deepcopy(terrain)
                invalid["flags"] = [flag]
                with self.assertRaises(ValidationError):
                    validate({"version": 1, "terrains": [invalid]}, self.schema)
        for action in ("", "open", "OPEN-", "OPEN\n"):
            with self.subTest(action=action):
                invalid = copy.deepcopy(terrain)
                invalid["interactions"] = {action: "FLOOR"}
                with self.assertRaises(ValidationError):
                    validate({"version": 1, "terrains": [invalid]}, self.schema)

    def test_drop_gold_semantics_require_positive_dice(self):
        for dice, accepted in (("0d0", False), ("0d1", False), ("1d0", False), ("-1d2", False), ("1d-2", False), ("1d1", True)):
            with self.subTest(dice=dice):
                terrain = self.terrain()
                terrain["interactions"] = {"DROP_GOLD": dice}
                document = {"terrains": [terrain]}
                if accepted:
                    validate_terrain_semantics(document, self.schema_path)
                else:
                    with self.assertRaises(ValidationError) as caught:
                        validate_terrain_semantics(document, self.schema_path)
                    self.assertEqual(list(caught.exception.path), ["terrains", 0, "interactions", "DROP_GOLD"])

    def test_drop_gold_schema_requires_complete_dice_string(self):
        for dice, accepted in (("1d2", True), ("001d002", True), ("1d2\n", False), ("1d2\r\n", False), ("1d2junk", False)):
            with self.subTest(dice=dice):
                terrain = self.terrain()
                terrain["interactions"] = {"DROP_GOLD": dice}
                if accepted:
                    validate({"version": 1, "terrains": [terrain]}, self.schema)
                else:
                    with self.assertRaises(ValidationError):
                        validate({"version": 1, "terrains": [terrain]}, self.schema)

    def test_empty_terrains_are_allowed(self):
        self.assert_valid([])

    def test_version_is_not_a_reader_integer_field(self):
        # version は 1 に限るが、reader が読まない項目なので、整数の表記 (1) でなく 1.0 と書いてもよい。
        ok, diagnostic = validate_document({"version": 1.0, "terrains": [self.terrain()]},
                                           self.schema_path, self.schema, self.registry)
        self.assertTrue(ok, diagnostic)
        ok, diagnostic = validate_document({"version": 1.5, "terrains": [self.terrain()]},
                                           self.schema_path, self.schema, self.registry)
        self.assertFalse(ok, diagnostic)
        self.assertIn("Location: ['version']", diagnostic)


if __name__ == "__main__":
    unittest.main()
