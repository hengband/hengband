"""JSONCのCI検証入口でモンスターメッセージ読込規則を確認する。"""
import copy
import json
from pathlib import Path
import tempfile
import unittest

from validate_json import build_schema_registry, load_all_schemas, load_jsonc, validate_one


class MonsterMessageValidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[2]
        cls.loaded, schema_map = load_all_schemas(cls.root / "schema")
        cls.schema_path = schema_map["MonsterMessages"]
        cls.registry = build_schema_registry(cls.loaded)

    def message(self, languages=("ja", "en"), chance=1):
        return {"action": "SPEAK_ALL", "chance": chance, "message": {language: ["Hello"] for language in languages}}

    def validate(self, groups):
        document = {"versions": 1, "groups": groups}
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "MonsterMessages.jsonc"
            target.write_text(json.dumps(document), encoding="utf-8")
            return validate_one((target, self.schema_path, self.loaded[self.schema_path]), self.registry)

    def assert_valid(self, groups):
        ok, diagnostic = self.validate(groups)
        self.assertTrue(ok, diagnostic)

    def assert_invalid(self, groups, path, reason=None):
        ok, diagnostic = self.validate(groups)
        self.assertFalse(ok, diagnostic)
        self.assertIn(f"Location: {path}", diagnostic)
        if reason:
            self.assertIn(reason, diagnostic)

    def test_bundled_messages(self):
        document = load_jsonc(self.root / "lib/edit/MonsterMessages.jsonc")
        self.assert_valid(document["groups"])

    def test_integer_boundaries(self):
        self.assert_valid([{"id_list": [1, 9999], "message": [self.message(chance=1), self.message(chance=100)]}])

    def test_float_id_is_rejected(self):
        self.assert_invalid([{"id_list": [1.0], "message": []}], ["groups", 0, "id_list", 0], "integer JSON value")

    def test_float_chance_is_rejected(self):
        self.assert_invalid([{"name": "DEFAULT", "message": [self.message(chance=1.0)]}],
                            ["groups", 0, "message", 0, "chance"], "integer JSON value")

    def test_other_invalid_ids_still_fail(self):
        for value in (True, "1", None, 1.5, 0, 10000):
            with self.subTest(value=value):
                self.assert_invalid([{"id_list": [value], "message": []}], ["groups", 0, "id_list", 0])

    def test_other_invalid_chances_still_fail(self):
        for value in (True, "1", None, 1.5, 0, 101):
            with self.subTest(value=value):
                self.assert_invalid([{"name": "DEFAULT", "message": [self.message(chance=value)]}],
                                    ["groups", 0, "message", 0, "chance"])

    def test_default_group(self):
        self.assert_valid([{"name": "DEFAULT", "message": [self.message()]}])

    def test_group_without_ids_requires_default_name(self):
        for name in (None, "default", "OTHER", ""):
            with self.subTest(name=name):
                group = {"message": []}
                if name is not None:
                    group["name"] = name
                self.assert_invalid([group], ["groups", 0, "name"], "must be named DEFAULT")

    def test_id_list_takes_precedence_over_name(self):
        for ids in ([], [9999]):
            with self.subTest(ids=ids):
                self.assert_valid([{"id_list": ids, "name": "OTHER", "message": [self.message()]}])

    def test_duplicate_ids_are_allowed(self):
        group = {"id_list": [1, 1], "message": [self.message()]}
        self.assert_valid([group, copy.deepcopy(group)])

    def test_empty_groups_and_messages_are_allowed(self):
        self.assert_valid([])
        self.assert_valid([{"name": "DEFAULT", "message": []}, {"id_list": [], "message": []}])

    def test_single_locale_is_allowed(self):
        for language in ("ja", "en"):
            with self.subTest(language=language):
                self.assert_valid([{"name": "DEFAULT", "message": [self.message((language,))]}])

    def test_empty_text_arrays_are_allowed(self):
        self.assert_valid([{"name": "DEFAULT", "message": [{"action": "SPEAK_ALL", "chance": 1,
                                                             "message": {"ja": [], "en": []}}]}])

    def test_optional_use_name(self):
        for value in (True, False):
            with self.subTest(value=value):
                message = self.message()
                message["use_name"] = value
                self.assert_valid([{"name": "DEFAULT", "message": [message]}])

    def test_missing_both_locales_is_rejected(self):
        self.assert_invalid([{"name": "DEFAULT", "message": [self.message(())]}],
                            ["groups", 0, "message", 0, "message"], "must contain ja or en")

    def test_one_reader_continues_after_other_reader_stops(self):
        for language in ("ja", "en"):
            for tail in (self.message(chance=1.0), self.message(())):
                with self.subTest(language=language, tail=tail):
                    groups = [{"name": "DEFAULT", "message": [self.message((language,)), tail]}]
                    field = "chance" if tail["message"] else "message"
                    self.assert_invalid(groups, ["groups", 0, "message", 1, field])

    def test_chance_is_checked_before_missing_locale_exit(self):
        for language, other in (("ja", "en"), ("en", "ja")):
            with self.subTest(language=language):
                self.assert_invalid([{"name": "DEFAULT", "message": [self.message((language,)),
                                                                     self.message((other,), chance=1.0)]}],
                                    ["groups", 0, "message", 1, "chance"], "integer JSON value")

    def test_semantic_checks_stop_after_both_readers_exit(self):
        for first, second in (("ja", "en"), ("en", "ja")):
            with self.subTest(first=first):
                self.assert_valid([{"name": "DEFAULT", "message": [self.message((first,)), self.message((second,)),
                                                                   self.message(chance=1.0), self.message(())]}])

    def test_language_reachability_resets_for_each_group(self):
        self.assert_invalid([{"name": "DEFAULT", "message": [self.message(("ja",)), self.message(("en",))]},
                             {"id_list": [1], "message": [self.message(chance=1.0)]}],
                            ["groups", 1, "message", 0, "chance"], "integer JSON value")

    def test_schema_still_checks_unreachable_messages(self):
        tail = self.message()
        tail["action"] = "UNKNOWN"
        self.assert_invalid([{"name": "DEFAULT", "message": [self.message(("ja",)), self.message(("en",)), tail]}],
                            ["groups", 0, "message", 2, "action"])


if __name__ == "__main__":
    unittest.main()
