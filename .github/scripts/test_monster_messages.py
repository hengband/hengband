"""JSONCのCI検証入口でモンスターメッセージ読込規則を確認する。"""
import copy
from pathlib import Path
import unittest

from json_validation_test_helper import validate_document
from validate_json import build_schema_registry, load_all_schemas, load_jsonc


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
        return validate_document(document, self.schema_path, self.loaded[self.schema_path], self.registry)

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
                self.assert_invalid([group], ["groups", 0] if name is None else ["groups", 0, "name"])

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

    def test_all_chances_are_checked_after_missing_locales(self):
        for language, other in (("ja", "en"), ("en", "ja")):
            with self.subTest(language=language):
                self.assert_invalid([{"name": "DEFAULT", "message": [self.message((language,)),
                                    self.message((other,)), self.message(chance=1.0)]}],
                                    ["groups", 0, "message", 2, "chance"], "integer JSON value")

    def test_empty_locales_are_checked_in_every_message(self):
        self.assert_invalid([{"name": "DEFAULT", "message": [self.message(("ja",)), self.message(("en",)),
                                                            self.message(())]}],
                            ["groups", 0, "message", 2, "message"], "must contain ja or en")

    def test_locale_cannot_return_after_gap(self):
        for language, other in (("ja", "en"), ("en", "ja")):
            for prefix in ([self.message((other,))], [self.message(), self.message((other,))]):
                with self.subTest(language=language, prefix=prefix):
                    self.assert_invalid([{"name": "DEFAULT", "message": prefix + [self.message()]}],
                                        ["groups", 0, "message", len(prefix), "message", language], "would be discarded")

    def test_single_locale_suffix_is_allowed(self):
        for language in ("ja", "en"):
            with self.subTest(language=language):
                self.assert_valid([{"name": "DEFAULT", "message": [self.message(), self.message((language,)),
                                                                 self.message((language,))]}])

    def test_empty_array_keeps_locale_present(self):
        first = self.message()
        first["message"]["en"] = []
        self.assert_valid([{"name": "DEFAULT", "message": [first, self.message()]}])

    def test_locale_tracking_resets_for_each_group(self):
        self.assert_valid([{"name": "DEFAULT", "message": [self.message(("ja",))]},
                           {"id_list": [1], "message": [self.message()]}])

    def test_schema_checks_all_messages(self):
        tail = self.message()
        tail["action"] = "UNKNOWN"
        self.assert_invalid([{"name": "DEFAULT", "message": [self.message(("ja",)), self.message(("en",)), tail]}],
                            ["groups", 0, "message", 2, "action"])

    def test_original_bundled_gaps_are_rejected(self):
        document = load_jsonc(self.root / "lib/edit/MonsterMessages.jsonc")
        for group_index, message_index, later_index in ((2, 1, 2), (3, 1, 2), (75, 0, 1)):
            with self.subTest(group=group_index):
                group = copy.deepcopy(document["groups"][group_index])
                del group["message"][message_index]["message"]["en"]
                self.assert_invalid([group], ["groups", 0, "message", later_index, "message", "en"], "would be discarded")


if __name__ == "__main__":
    unittest.main()
