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


if __name__ == "__main__":
    unittest.main()
