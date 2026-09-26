"""Regression tests for the actual JSONC validation entry point used in CI."""
import copy
import json
from pathlib import Path
import tempfile
import unittest

from validate_json import load_jsonc, validate_one


class VaultValidationTest(unittest.TestCase):
    def setUp(self):
        self.schema_path = Path(__file__).resolve().parents[2] / "schema/VaultDefinitions.schema.json"
        self.schema = load_jsonc(self.schema_path)
        self.record = {
            "id": 0, "name": "Test", "type": 7, "rating": 5,
            "height": 1, "width": 3, "layout": [" % "],
        }

    def validate(self, data):
        """Use a real temporary JSON file, including schema and semantic checks."""
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "VaultDefinitions.jsonc"
            target.write_text(json.dumps(data), encoding="utf-8")
            return validate_one((target, self.schema_path, self.schema))

    def test_valid_sparse_ids_and_spaces(self):
        second = {**self.record, "id": 3}
        ok, message = self.validate({"vaults": [self.record, second]})
        self.assertTrue(ok, message)

    def test_invalid_roots(self):
        for root in (None, [], {}, {"vaults": None}, {"vaults": {}}, {"vaults": []}):
            with self.subTest(root=root):
                self.assertFalse(self.validate(root)[0])

    def test_duplicate_and_descending_ids(self):
        for ids in ((0, 0), (2, 1)):
            with self.subTest(ids=ids):
                records = [{**self.record, "id": index} for index in ids]
                ok, message = self.validate({"vaults": records})
                self.assertFalse(ok)
                self.assertIn("increasing order", message)
                self.assertIn("['vaults', 1, 'id']", message)

    def test_layout_dimensions(self):
        for layout, reason in (([" % ", " % "], "row count"), (["%"], "row byte length"), (["%%%%"], "row byte length")):
            with self.subTest(layout=layout):
                record = {**self.record, "layout": layout}
                ok, message = self.validate({"vaults": [record]})
                self.assertFalse(ok)
                self.assertIn(reason, message)
                self.assertIn("['vaults', 0, 'layout'", message)

    def test_non_ascii_and_control_characters(self):
        for row in ("%\n", "\u00e9% ", "%\t ", "%\x00 "):
            with self.subTest(row=row):
                record = {**self.record, "layout": [row]}
                self.assertFalse(self.validate({"vaults": [record]})[0])

    def test_integer_representation_matches_reader(self):
        for field in ("id", "type", "rating", "height", "width"):
            with self.subTest(field=field):
                record = copy.deepcopy(self.record)
                record[field] = float(record[field])
                ok, message = self.validate({"vaults": [record]})
                self.assertFalse(ok)
                self.assertIn("integer JSON value", message)

    def test_other_schemas_do_not_require_vault_fields(self):
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "Other.jsonc"
            target.write_text('{"items": []}', encoding="utf-8")
            schema = {"type": "object", "properties": {"items": {"type": "array"}}}
            ok, message = validate_one((target, Path("Other.schema.json"), schema))
            self.assertTrue(ok, message)


class EgoValidationTest(unittest.TestCase):
    def setUp(self):
        self.schema_path = Path(__file__).resolve().parents[2] / "schema/EgoDefinitions.schema.json"
        self.schema = load_jsonc(self.schema_path)
        self.record = {
            "id": 4,
            "name": {"ja": "試験の", "en": "of Testing"},
            "slot": 31,
            "rating": 10,
            "level": 0,
            "rarity": 20,
            "cost": 1000,
            "maximum_bonuses": {"to_hit": 1, "to_damage": 2, "to_ac": 3, "pval": 4},
            "flags": ["STR"],
            "extra_flags": [{"numerator": 1, "denominator": 3, "flags": ["RES_FIRE"]}],
        }

    def validate(self, data):
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "EgoDefinitions.jsonc"
            target.write_text(json.dumps(data), encoding="utf-8")
            return validate_one((target, self.schema_path, self.schema))

    def test_valid_sparse_ids(self):
        second = {**self.record, "id": 8}
        ok, message = self.validate({"version": 1, "egos": [self.record, second]})
        self.assertTrue(ok, message)

    def test_duplicate_ids(self):
        records = [self.record, {**self.record}]
        ok, message = self.validate({"version": 1, "egos": records})
        self.assertFalse(ok)
        self.assertIn("unique", message)

    def test_descending_ids_are_supported(self):
        records = [{**self.record, "id": ego_id} for ego_id in (8, 4)]
        ok, message = self.validate({"version": 1, "egos": records})
        self.assertTrue(ok, message)

    def test_integer_representation_matches_reader(self):
        for field in ("id", "slot", "rating", "level", "rarity", "cost"):
            with self.subTest(field=field):
                record = copy.deepcopy(self.record)
                record[field] = float(record[field])
                ok, message = self.validate({"version": 1, "egos": [record]})
                self.assertFalse(ok)
                self.assertIn("integer JSON value", message)

    def test_rarity_matches_byte_storage(self):
        record = {**self.record, "rarity": 255}
        ok, message = self.validate({"version": 1, "egos": [record]})
        self.assertTrue(ok, message)
        record["rarity"] = 256
        ok, message = self.validate({"version": 1, "egos": [record]})
        self.assertFalse(ok)

    def test_level_matches_depth_storage(self):
        record = {**self.record, "level": 32768}
        ok, message = self.validate({"version": 1, "egos": [record]})
        self.assertTrue(ok, message)
        record["level"] = 2147483648
        ok, message = self.validate({"version": 1, "egos": [record]})
        self.assertFalse(ok)

    def test_known_runtime_tokens_are_accepted_by_ci(self):
        record = copy.deepcopy(self.record)
        record["activation"] = "SUNLIGHT"
        record["flags"] = ["RES_FIRE", "CURSED"]
        record["extra_flags"][0]["flags"] = ["RES_COLD", "HEAVY_CURSE"]
        ok, message = self.validate({"version": 1, "egos": [record]})
        self.assertTrue(ok, message)

    def test_duplicate_flags_are_rejected_by_ci(self):
        for path in (("flags",), ("extra_flags", 0, "flags")):
            with self.subTest(path=path):
                record = copy.deepcopy(self.record)
                target = record
                for part in path[:-1]:
                    target = target[part]
                target[path[-1]] = ["RES_FIRE", "RES_FIRE"]
                ok, message = self.validate({"version": 1, "egos": [record]})
                self.assertFalse(ok)
                self.assertIn("non-unique", message)

    def test_unknown_runtime_tokens_are_rejected_by_ci(self):
        for path, value in (
            (("activation",), "UNKNOWN"),
            (("flags", 0), "UNKNOWN"),
            (("extra_flags", 0, "flags", 0), "UNKNOWN"),
        ):
            with self.subTest(path=path):
                record = copy.deepcopy(self.record)
                target = record
                for part in path[:-1]:
                    target = target[part]
                target[path[-1]] = value
                ok, message = self.validate({"version": 1, "egos": [record]})
                self.assertFalse(ok)
                self.assertIn("unknown", message)

    def test_nested_integer_representation_matches_reader(self):
        for path in (
            ("base_bonuses", "to_hit"),
            ("maximum_bonuses", "pval"),
            ("extra_flags", 0, "numerator"),
            ("extra_flags", 0, "denominator"),
        ):
            with self.subTest(path=path):
                record = copy.deepcopy(self.record)
                record.setdefault("base_bonuses", {"to_hit": 1, "to_damage": 2, "to_ac": 3})
                target = record
                for part in path[:-1]:
                    target = target[part]
                target[path[-1]] = float(target[path[-1]])
                ok, message = self.validate({"version": 1, "egos": [record]})
                self.assertFalse(ok)


class WildernessValidationTest(unittest.TestCase):
    def setUp(self):
        self.schema_path = Path(__file__).resolve().parents[2] / "schema/WildernessDefinition.schema.json"
        self.schema = load_jsonc(self.schema_path)
        self.data = {
            "version": 1,
            "width": 3,
            "height": 3,
            "towns": [{"id": 1, "name": {"ja": "街", "en": "Town"}}],
            "maps": {
                "normal": {
                    "letters": [
                        {"symbol": "#", "terrain": 0},
                        {"symbol": "1", "terrain": 1, "level": {"ja": 30, "en": 20}, "town": 1},
                    ],
                    "layout": ["###", "#1#", "###"],
                    "starting_position": {"x": 1, "y": 1},
                },
                "compact": {
                    "letters": [{"symbol": "#", "terrain": 0}],
                    "layout": ["###", "###", "###"],
                    "starting_position": {"x": 1, "y": 1},
                },
            },
        }

    def validate(self, data):
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "WildernessDefinition.jsonc"
            target.write_text(json.dumps(data), encoding="utf-8")
            return validate_one((target, self.schema_path, self.schema))

    def test_valid_localized_level_and_compact_map(self):
        ok, message = self.validate(self.data)
        self.assertTrue(ok, message)

    def test_duplicate_town_ids_and_symbols(self):
        duplicate_town = copy.deepcopy(self.data)
        duplicate_town["towns"].append(copy.deepcopy(duplicate_town["towns"][0]))
        self.assertFalse(self.validate(duplicate_town)[0])

        duplicate_symbol = copy.deepcopy(self.data)
        duplicate_symbol["maps"]["normal"]["letters"].append({"symbol": "#", "terrain": 0})
        self.assertFalse(self.validate(duplicate_symbol)[0])

    def test_undefined_town_references(self):
        for map_name in ("normal", "compact"):
            with self.subTest(map_name=map_name):
                data = copy.deepcopy(self.data)
                data["maps"][map_name]["letters"][0]["town"] = 2
                ok, message = self.validate(data)
                self.assertFalse(ok)
                self.assertIn("undefined town ID", message)

    def test_normal_dimensions_and_defined_symbols(self):
        short_row = copy.deepcopy(self.data)
        short_row["maps"]["normal"]["layout"][1] = "##"
        self.assertFalse(self.validate(short_row)[0])

        undefined_symbol = copy.deepcopy(self.data)
        undefined_symbol["maps"]["normal"]["layout"][1] = "#.#"
        self.assertFalse(self.validate(undefined_symbol)[0])

    def test_starting_position_is_inside_layout(self):
        outside = copy.deepcopy(self.data)
        outside["maps"]["compact"]["starting_position"] = {"x": 3, "y": 1}
        self.assertFalse(self.validate(outside)[0])

    def test_integer_representation_matches_reader(self):
        for field in ("width", "height"):
            with self.subTest(field=field):
                data = copy.deepcopy(self.data)
                data[field] = float(data[field])
                self.assertFalse(self.validate(data)[0])


class TownPreferencesValidationTest(unittest.TestCase):
    def setUp(self):
        self.schema_path = Path(__file__).resolve().parents[2] / "schema/TownPreferences.schema.json"
        self.schema = load_jsonc(self.schema_path)

    def validate(self, data):
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "TownPreferences.jsonc"
            target.write_text(json.dumps(data), encoding="utf-8")
            return validate_one((target, self.schema_path, self.schema))

    def test_reader_integer_types(self):
        data = {"version": 1, "legend": {">": {"terrain": "ENTRANCE", "caveInfo": ["MARK"], "special": 2}}}
        self.assertTrue(self.validate(data)[0])

        for field, value in (("version", 1.0), ("special", 2.0)):
            with self.subTest(field=field):
                invalid = copy.deepcopy(data)
                if field == "version":
                    invalid["version"] = value
                else:
                    invalid["legend"][">"]["special"] = value
                ok, message = self.validate(invalid)
                self.assertFalse(ok)
                self.assertIn("integer JSON value", message)

    def test_terrain_tags_match_runtime_definitions(self):
        for terrain, valid in (("ENTRANCE", True), ("*", True), ("NOT_A_TERRAIN", False)):
            with self.subTest(terrain=terrain):
                data = {"version": 1, "legend": {">": {"terrain": terrain, "caveInfo": ["MARK"]}}}
                ok, message = self.validate(data)
                self.assertEqual(ok, valid, message)
                if not valid:
                    self.assertIn("unknown terrain tag", message)
                    self.assertIn("['legend', '>', 'terrain']", message)


class TownDefinitionListValidationTest(unittest.TestCase):
    def setUp(self):
        self.schema_path = Path(__file__).resolve().parents[2] / "schema/TownDefinitionList.schema.json"
        self.schema = load_jsonc(self.schema_path)
        self.data = {
            "version": 1,
            "towns": {
                "1": {
                    "lite": "towns/01_Outpost_Lite.jsonc",
                    "normal": "towns/01_Outpost_Full.jsonc",
                    "none": "towns/01_Outpost_OnlyAngband.jsonc",
                },
                "2": "towns/02_Telmora.jsonc",
            },
        }

    def validate(self, data):
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "TownDefinitionList.jsonc"
            target.write_text(json.dumps(data), encoding="utf-8")
            return validate_one((target, self.schema_path, self.schema))

    def test_valid_map_variants(self):
        ok, message = self.validate(self.data)
        self.assertTrue(ok, message)

    def test_missing_or_unknown_map_file(self):
        for town, mode in (("1", "lite"), ("2", None)):
            with self.subTest(town=town, mode=mode):
                invalid = copy.deepcopy(self.data)
                if mode:
                    invalid["towns"][town][mode] = "towns/Missing.jsonc"
                else:
                    invalid["towns"][town] = "towns/Missing.jsonc"
                ok, message = self.validate(invalid)
                self.assertFalse(ok)
                self.assertIn("town map file does not exist", message)

    def test_invalid_map_mode_and_path(self):
        for replacement in ({"normal": "towns/01_Outpost_Full.txt"}, "../outside.txt"):
            with self.subTest(replacement=replacement):
                invalid = copy.deepcopy(self.data)
                invalid["towns"]["1"] = replacement
                self.assertFalse(self.validate(invalid)[0])

    def test_reader_integer_version(self):
        invalid = copy.deepcopy(self.data)
        invalid["version"] = 1.0
        ok, message = self.validate(invalid)
        self.assertFalse(ok)
        self.assertIn("integer JSON value", message)


class TownMapValidationTest(unittest.TestCase):
    def setUp(self):
        self.schema_path = Path(__file__).resolve().parents[2] / "schema/TownMap.schema.json"
        self.schema = load_jsonc(self.schema_path)
        self.data = {
            "version": 2,
            "featureRules": [
                {
                    "when": "[EQU $QUEST1 1]",
                    "symbol": "#",
                    "definition": {
                        "terrain": "PERMANENT",
                        "caveInfo": 3,
                        "monster": "0",
                        "object": "0",
                        "ego": "0",
                        "artifact": "0",
                        "trap": "NONE",
                        "special": 0,
                    },
                }
            ],
            "mapVariants": [{"rows": ["###", "#.#", "###"]}],
            "buildingRules": [{"index": 0, "locale": "en", "command": "N", "fields": ["Store", "Owner", "Human"]}],
            "startingPositions": [{"y": 1, "x": 1}],
        }

    def validate(self, data):
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "TestTown.jsonc"
            target.write_text(json.dumps(data), encoding="utf-8")
            return validate_one((target, self.schema_path, self.schema))

    def test_valid_map_and_start(self):
        ok, message = self.validate(self.data)
        self.assertTrue(ok, message)

    def test_start_must_be_inside_layout(self):
        invalid = copy.deepcopy(self.data)
        invalid["startingPositions"][0] = {"y": 3, "x": 1}
        ok, message = self.validate(invalid)
        self.assertFalse(ok)
        self.assertIn("starting position must be within every map variant", message)

    def test_rows_must_have_equal_widths(self):
        invalid = copy.deepcopy(self.data)
        invalid["mapVariants"][0]["rows"][1] = "##"
        ok, message = self.validate(invalid)
        self.assertFalse(ok)
        self.assertIn("map rows must have equal widths", message)

    def test_map_must_fit_runtime_height_limit(self):
        invalid = copy.deepcopy(self.data)
        invalid["mapVariants"][0]["rows"] = ["###"] * 67
        ok, message = self.validate(invalid)
        self.assertFalse(ok)
        self.assertIn("map height exceeds the reader's maximum", message)

    def test_map_must_fit_runtime_width_limit(self):
        invalid = copy.deepcopy(self.data)
        invalid["mapVariants"][0]["rows"] = ["#" * 199]
        ok, message = self.validate(invalid)
        self.assertFalse(ok)
        self.assertIn("map width exceeds the reader's maximum", message)

    def test_invalid_building_command_is_rejected(self):
        invalid = copy.deepcopy(self.data)
        invalid["buildingRules"][0]["command"] = "UNKNOWN"
        self.assertFalse(self.validate(invalid)[0])

    def test_colon_in_feature_symbol_is_rejected(self):
        invalid = copy.deepcopy(self.data)
        invalid["featureRules"][0]["symbol"] = ":"
        self.assertFalse(self.validate(invalid)[0])

    def test_tokenizer_delimiters_in_feature_symbol_are_rejected(self):
        for symbol in ("/", "\\"):
            with self.subTest(symbol=symbol):
                invalid = copy.deepcopy(self.data)
                invalid["featureRules"][0]["symbol"] = symbol
                self.assertFalse(self.validate(invalid)[0])

    def test_colon_in_building_field_is_rejected(self):
        invalid = copy.deepcopy(self.data)
        invalid["buildingRules"][0]["fields"][0] = "Store: Annex"
        self.assertFalse(self.validate(invalid)[0])

    def test_slash_in_building_field_is_rejected(self):
        invalid = copy.deepcopy(self.data)
        invalid["buildingRules"][0]["fields"][0] = "Buy/Sell"
        self.assertFalse(self.validate(invalid)[0])

    def test_backslash_in_building_field_is_rejected(self):
        invalid = copy.deepcopy(self.data)
        invalid["buildingRules"][0]["fields"][0] = "Action\\"
        self.assertFalse(self.validate(invalid)[0])

    def test_too_many_building_membership_fields_are_rejected(self):
        for command, maximum in (("C", 29), ("M", 10), ("R", 38)):
            with self.subTest(command=command):
                invalid = copy.deepcopy(self.data)
                invalid["buildingRules"][0].update(command=command, fields=["0"] * (maximum + 1))
                ok, message = self.validate(invalid)
                self.assertFalse(ok)
                self.assertIn("too many fields", message)

    def test_empty_building_membership_fields_are_rejected(self):
        for command in ("C", "M", "R"):
            with self.subTest(command=command):
                invalid = copy.deepcopy(self.data)
                invalid["buildingRules"][0].update(command=command, fields=[])
                ok, message = self.validate(invalid)
                self.assertFalse(ok)
                self.assertIn("requires at least one field", message)

    def test_building_action_index_is_rejected_outside_reader_range(self):
        for index in ("-1", "8"):
            with self.subTest(index=index):
                invalid = copy.deepcopy(self.data)
                invalid["buildingRules"][0].update(command="A", fields=[index, "Action", "0", "0", "a", "0", "0"])
                self.assertFalse(self.validate(invalid)[0])

    def test_colon_in_feature_token_is_rejected(self):
        invalid = copy.deepcopy(self.data)
        invalid["featureRules"][0]["definition"]["monster"] = "1:2"
        self.assertFalse(self.validate(invalid)[0])

    def test_tokenizer_delimiters_in_feature_fields_are_rejected(self):
        for field in ("terrain", "monster", "object", "ego", "artifact", "trap"):
            for delimiter in ("/", "\\"):
                with self.subTest(field=field, delimiter=delimiter):
                    invalid = copy.deepcopy(self.data)
                    invalid["featureRules"][0]["definition"][field] += delimiter
                    self.assertFalse(self.validate(invalid)[0])

    def test_start_must_fit_every_map_variant(self):
        invalid = copy.deepcopy(self.data)
        invalid["mapVariants"][0]["when"] = "[EQU $QUEST1 0]"
        invalid["mapVariants"].append({"when": "[EQU $QUEST1 1]", "rows": ["##", "##"]})
        invalid["startingPositions"][0] = {"y": 1, "x": 2}
        ok, message = self.validate(invalid)
        self.assertFalse(ok)
        self.assertIn("within every map variant", message)

    def test_multiple_map_variants_require_conditions(self):
        invalid = copy.deepcopy(self.data)
        invalid["mapVariants"].append({"when": "[EQU $QUEST1 1]", "rows": ["###", "#.#", "###"]})
        ok, message = self.validate(invalid)
        self.assertFalse(ok)
        self.assertIn("every map variant requires when", message)

    def test_integer_representation_matches_reader(self):
        cases = {
            "version": lambda data: data.update(version=2.0),
            "starting position y": lambda data: data["startingPositions"][0].update(y=1.0),
            "starting position x": lambda data: data["startingPositions"][0].update(x=1.0),
            "caveInfo": lambda data: data["featureRules"][0]["definition"].update(caveInfo=3.0),
            "special": lambda data: data["featureRules"][0]["definition"].update(special=0.0),
            "building index": lambda data: data["buildingRules"][0].update(index=0.0),
        }
        for field, set_float_value in cases.items():
            with self.subTest(field=field):
                invalid = copy.deepcopy(self.data)
                set_float_value(invalid)
                ok, message = self.validate(invalid)
                self.assertFalse(ok)
                self.assertIn("integer JSON value", message)

    def test_building_numeric_fields_fit_reader_integer_range(self):
        cases = (
            ("A", ["0", "Action", "0", "0", "a", "0", "0"], 2),
            ("C", ["0"], 0),
        )
        for command, fields, field_index in cases:
            with self.subTest(command=command):
                invalid = copy.deepcopy(self.data)
                invalid["buildingRules"][0].update(command=command, fields=fields)
                invalid["buildingRules"][0]["fields"][field_index] = "2147483648"
                ok, message = self.validate(invalid)
                self.assertFalse(ok)
                self.assertIn("building field exceeds the reader's integer range", message)

    def test_numeric_tokens_require_ascii_digits(self):
        invalid = copy.deepcopy(self.data)
        invalid["featureRules"][0]["definition"]["monster"] = "١"
        self.assertFalse(self.validate(invalid)[0])

    def test_feature_tokens_match_reader_grammar_and_storage_ranges(self):
        cases = (
            ("monster", "invalid"),
            ("monster", "*x"),
            ("monster", "cX"),
            ("monster", "32768"),
            ("monster", "-32768"),
            ("monster", "*32768"),
            ("monster", "c32768"),
            ("monster", "c-0"),
            ("object", "32768"),
            ("object", "*32768"),
            ("artifact", "32768"),
            ("artifact", "*32768"),
            ("ego", "2147483648"),
            ("artifact", "2147483648"),
        )
        for field, token in cases:
            with self.subTest(field=field, token=token):
                invalid = copy.deepcopy(self.data)
                invalid["featureRules"][0]["definition"][field] = token
                self.assertFalse(self.validate(invalid)[0])


if __name__ == "__main__":
    unittest.main()
