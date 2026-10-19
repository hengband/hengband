"""クエスト定義の schema が QuestReader の int16 の範囲と一致することを確かめる。"""
import copy
from pathlib import Path
import unittest

from json_validation_test_helper import validate_document
from validate_json import build_schema_registry, load_all_schemas


class QuestInt16RangeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        schema_dir = Path(__file__).resolve().parents[2] / "schema"
        cls.loaded, cls.schema_map = load_all_schemas(schema_dir)
        cls.registry = build_schema_registry(cls.loaded)

    def validate(self, schema_name, document):
        schema_path = self.schema_map[schema_name]
        return validate_document(document, schema_path, self.loaded[schema_path], self.registry, filename="Test.jsonc")

    def quest(self, cell=None, definition=None):
        document = {
            "version": 1,
            "id": 1,
            "name": {"ja": "試験", "en": "Test"},
            "definition": {"type": "KILL_ALL", "level": 1},
            "legend": {"#": cell or {}},
        }
        document["definition"].update(definition or {})
        return document

    def assert_rejected_by_range(self, message, rejected):
        # oneOf の中の項目は jsonschema の診断の位置が分岐からの相対になるため、範囲の理由で確かめる。
        reason = "is greater than the maximum of 32767" if rejected > 0 else "is less than the minimum of -32768"
        self.assertIn(f"{rejected} {reason}", message)

    def assert_boundary(self, make, accepted, rejected, location=None):
        ok, message = self.validate("Quest", make(accepted))
        self.assertTrue(ok, message)
        ok, message = self.validate("Quest", make(rejected))
        self.assertFalse(ok, message)
        self.assert_rejected_by_range(message, rejected)
        # oneOf の外の項目は診断の位置が安定しているため、上限が意図した項目に付いていることも確かめる。
        if location is not None:
            self.assertIn(f"Location: {location}", message)

    def test_legend_fields_follow_reader_int16_range(self):
        cases = (
            lambda value: {"monster": value},
            lambda value: {"monster": {"random": True, "oodLevel": value}},
            lambda value: {"monster": {"cloneOf": value}},
            lambda value: {"object": value},
            lambda value: {"object": {"random": True, "oodLevel": value}},
            lambda value: {"artifact": value},
            lambda value: {"artifact": {"random": True, "id": value}},
        )
        for make_cell in cases:
            with self.subTest(cell=make_cell(32768)):
                self.assert_boundary(lambda value: self.quest(make_cell(value)), 32767, 32768)

    def test_special_follows_reader_signed_int16_range(self):
        for accepted, rejected in ((32767, 32768), (-32768, -32769)):
            with self.subTest(rejected=rejected):
                self.assert_boundary(lambda value: self.quest({"special": value}), accepted, rejected, ["legend", "#", "special"])

    def test_definition_ids_follow_reader_int16_range(self):
        cases = (
            (lambda value: {"monster": value}, ["definition", "monster"]),
            (lambda value: {"reward": {"artifact": value}}, ["definition", "reward", "artifact"]),
            (lambda value: {"reward": {"artifacts": [1, value]}}, ["definition", "reward", "artifacts", 1]),
        )
        for make_definition, location in cases:
            with self.subTest(definition=make_definition(32768)):
                self.assert_boundary(lambda value: self.quest(definition=make_definition(value)), 32767, 32768, location)

    def test_level_and_kill_count_follow_reader_ranges(self):
        # 階層は MAX_DEPTH - 1 まで、撃破数はセーブの16ビットまでしか読み込まない。
        for field, maximum in (("level", 127), ("maxNum", 32767)):
            for value, accepted in ((0, True), (maximum, True), (maximum + 1, False), (-1, False)):
                with self.subTest(field=field, value=value):
                    ok, message = self.validate("Quest", self.quest(definition={field: value}))
                    self.assertEqual(ok, accepted, message)
                    if not accepted:
                        self.assertIn(f"Location: {['definition', field]}", message)

    def test_other_integers_follow_reader_ranges(self):
        # QuestReader は int に収まらない値を拒否し、開始位置は地図（最大 66 行 198 列）の中に限る。
        int_max = 2147483647
        full_map = ["#" * 198] * 66
        cases = (
            ("numMon", int_max, lambda value: self.quest(definition={"numMon": value}), ["definition", "numMon"]),
            ("dungeon", int_max, lambda value: self.quest(definition={"dungeon": value}), ["definition", "dungeon"]),
            ("start.y", 65, lambda value: {**self.quest(), "map": full_map, "start": {"y": value, "x": 0}}, ["start", "y"]),
            ("start.x", 197, lambda value: {**self.quest(), "map": full_map, "start": {"y": 0, "x": value}}, ["start", "x"]),
            ("leavingQuest", int_max,
             lambda value: {**self.quest(), "map": full_map, "startVariants": [{"leavingQuest": value, "y": 0, "x": 0}]},
             ["startVariants", 0, "leavingQuest"]),
            ("startVariants.y", 65, lambda value: {**self.quest(), "map": full_map, "startVariants": [{"y": value, "x": 0}]},
             ["startVariants", 0, "y"]),
            ("startVariants.x", 197, lambda value: {**self.quest(), "map": full_map, "startVariants": [{"y": 0, "x": value}]},
             ["startVariants", 0, "x"]),
        )
        for name, maximum, make, location in cases:
            with self.subTest(field=name):
                ok, message = self.validate("Quest", make(maximum))
                # leavingQuest の上限は参照先が無いため、範囲の検査だけを確かめる。
                if name != "leavingQuest":
                    self.assertTrue(ok, message)
                ok, message = self.validate("Quest", make(maximum + 1))
                self.assertFalse(ok, message)
                self.assertIn(f"{maximum + 1} is greater than the maximum of {maximum}", message)
                self.assertIn(f"Location: {location}", message)

    def test_quest_preferences_share_the_legend_ranges(self):
        document = {"version": 1, "legend": {"#": {"monster": 32767, "special": -32768}}}
        ok, message = self.validate("QuestPreferences", document)
        self.assertTrue(ok, message)
        for cell, rejected, location in (({"monster": 32768}, 32768, None), ({"special": -32769}, -32769, ["legend", "#", "special"])):
            with self.subTest(cell=cell):
                invalid = copy.deepcopy(document)
                invalid["legend"]["#"] = cell
                ok, message = self.validate("QuestPreferences", invalid)
                self.assertFalse(ok, message)
                self.assert_rejected_by_range(message, rejected)
                if location is not None:
                    self.assertIn(f"Location: {location}", message)


class QuestLayoutTest(unittest.TestCase):
    """CI が QuestReader と同じく地図・凡例・開始位置を拒否することを確かめる。"""

    @classmethod
    def setUpClass(cls):
        schema_dir = Path(__file__).resolve().parents[2] / "schema"
        cls.loaded, cls.schema_map = load_all_schemas(schema_dir)
        cls.registry = build_schema_registry(cls.loaded)

    def validate(self, schema_name, document):
        schema_path = self.schema_map[schema_name]
        return validate_document(document, schema_path, self.loaded[schema_path], self.registry, filename="Test.jsonc")

    def quest(self, **fields):
        document = {
            "version": 1,
            "id": 1,
            "name": {"ja": "試験", "en": "Test"},
            "definition": {"type": "KILL_ALL", "level": 1},
            "legend": {"#": {}, "+": {}},
            "map": ["###", "#+#", "###"],
        }
        document.update(fields)
        return document

    def variants(self, variants, **fields):
        """map の代わりに mapVariants を持つクエストを作る。"""
        document = self.quest(mapVariants=variants, **fields)
        del document["map"]
        return document

    def assert_rejected(self, document, reason, location, schema_name="Quest"):
        ok, message = self.validate(schema_name, document)
        self.assertFalse(ok, message)
        self.assertIn(reason, message)
        self.assertIn(f"Location: {location}", message)

    def test_valid_layout_is_accepted(self):
        document = self.quest(start={"y": 2, "x": 2})
        ok, message = self.validate("Quest", document)
        self.assertTrue(ok, message)

    def test_legend_symbols_must_be_one_printable_ascii_character(self):
        for symbol in ("\u00e9", "\x7f", "\x1f", "\n"):
            for schema_name in ("Quest", "QuestPreferences"):
                with self.subTest(symbol=repr(symbol), schema=schema_name):
                    document = self.quest() if schema_name == "Quest" else {"version": 1, "legend": {}}
                    document["legend"][symbol] = {}
                    self.assert_rejected(document, "legend symbol must be exactly one printable ASCII character",
                                         ["legend", symbol], schema_name)
        for symbol in (" ", "~"):
            with self.subTest(symbol=symbol):
                document = self.quest()
                document["legend"][symbol] = {}
                ok, message = self.validate("Quest", document)
                self.assertTrue(ok, message)

    def test_map_rows_must_be_printable_ascii(self):
        for character in ("\u00e9", "\x7f", "\t", "\x00"):
            with self.subTest(character=repr(character)):
                self.assert_rejected(self.quest(map=["###", "#" + character + "#"]),
                                     "map rows must contain printable ASCII characters", ["map", 1, 1])
        self.assert_rejected(self.variants([["###"], ["##\u00e9"]]),
                             "map rows must contain printable ASCII characters", ["mapVariants", 1, 0, 2])

    def test_map_rows_must_have_the_same_width_within_each_variant(self):
        self.assert_rejected(self.quest(map=["###", "##", "###"]), "map rows must have the same width", ["map", 1])
        # 生成側は最終行の長さから幅を決めるため、最終行だけ短い地図も拒否する。
        self.assert_rejected(self.quest(map=["###", "###", "##"]), "map rows must have the same width", ["map", 2])
        variants = self.variants([["##", "##"], ["###", "##"]])
        self.assert_rejected(variants, "map rows must have the same width", ["mapVariants", 1, 1])
        # 候補ごとに幅が違うのはよい。
        variants = self.variants([["##", "##"], ["###"]])
        ok, message = self.validate("Quest", variants)
        self.assertTrue(ok, message)

    def test_map_takes_precedence_over_unused_variants(self):
        # QuestReader は map があれば mapVariants を読まない。
        ok, message = self.validate("Quest", self.quest(mapVariants=[["#"], ["##", "#"]], start={"y": 2, "x": 2}))
        self.assertTrue(ok, message)

    def test_start_must_be_within_every_map_variant(self):
        variants = self.variants([["###", "###"], ["##", "##", "##"]])
        for start, accepted in (({"y": 1, "x": 1}, True), ({"y": 1, "x": 2}, False), ({"y": 2, "x": 0}, False)):
            with self.subTest(start=start):
                document = variants | {"start": start}
                ok, message = self.validate("Quest", document)
                self.assertEqual(ok, accepted, message)
                if not accepted:
                    self.assertIn("start position must be within every map variant", message)
                    self.assertIn("Location: ['start']", message)
        # 条件付きの開始位置も、条件に関係なくすべての候補で確かめる。
        document = variants | {"startVariants": [{"y": 0, "x": 0}, {"leavingQuest": 1, "y": 0, "x": 2}]}
        self.assert_rejected(document, "start position must be within every map variant", ["startVariants", 1])
        # 空の行しか無い地図には開始位置を置けない。
        self.assert_rejected(self.quest(map=[""], start={"y": 0, "x": 0}), "start position must be within every map variant", ["start"])

    def test_start_takes_precedence_over_unused_start_variants(self):
        document = self.quest(start={"y": 0, "x": 0}, startVariants=[{"y": 9, "x": 9}])
        ok, message = self.validate("Quest", document)
        self.assertTrue(ok, message)


if __name__ == "__main__":
    unittest.main()
