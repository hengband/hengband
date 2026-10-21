"""JSONCのCI検証入口でモンスターメッセージ読込規則を確認する。"""
import copy
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from json_validation_test_helper import monrace_id_schema_maximum, validate_document
from validate_json import (build_schema_registry, load_all_schemas, load_jsonc,
                           load_validated_companion, validate_one, validate_monster_message_semantics)


class MonsterMessageValidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[2]
        cls.loaded, schema_map = load_all_schemas(cls.root / "schema")
        cls.schema_path = schema_map["MonsterMessages"]
        cls.registry = build_schema_registry(cls.loaded)
        monrace_document = load_jsonc(cls.root / "lib/edit/MonraceDefinitions.jsonc")
        monraces = monrace_document["monsters"]
        cls.monrace_version = monrace_document["versions"]
        cls.monrace_template = copy.deepcopy(monraces[1])
        cls.other_monrace_id = monraces[2]["id"]
        cls.max_monrace_id = max(monrace["id"] for monrace in monraces)


    def test_malformed_monster_definitions_are_reported(self):
        # 参照先の欠落・型不正を例外のままにせず、対象ファイルの診断として返す。
        for companion in ([], {}, {"monsters": None}, {"monsters": [{}]}):
            with self.subTest(companion=companion):
                ok, diagnostic = self.validate([], companions={"MonraceDefinitions.jsonc": companion})
                self.assertFalse(ok, diagnostic)
                self.assertIn("Invalid monster definitions", diagnostic)
                self.assertIn("MonraceDefinitions.jsonc", diagnostic)

    def test_adjacent_monster_definitions_take_precedence(self):
        monster = dict(self.monrace_template, id=self.max_monrace_id + 1)
        companion = {"versions": self.monrace_version, "monsters": [monster]}
        ok, diagnostic = self.validate([{"id_list": [monster["id"]], "message": []}],
                                       companions={"MonraceDefinitions.jsonc": companion})
        self.assertTrue(ok, diagnostic)
        ok, diagnostic = self.validate([{"id_list": [1], "message": []}],
                                       companions={"MonraceDefinitions.jsonc": companion})
        self.assertFalse(ok, diagnostic)
        self.assertIn("unknown monster ID", diagnostic)

    def test_monster_id_cache_reuses_validation_and_observes_same_path_updates(self):
        with tempfile.TemporaryDirectory() as folder:
            directory = Path(folder)
            target = directory / "MonsterMessages.jsonc"
            companion = directory / "MonraceDefinitions.jsonc"
            message = {"versions": 1, "groups": [{"id_list": [1], "message": []}]}
            target.write_text(json.dumps(message), encoding="utf-8")
            document = {"versions": self.monrace_version, "monsters": [dict(self.monrace_template, id=1)]}
            original = json.dumps(document)
            companion.write_text(original, encoding="utf-8")
            pair = (target, self.schema_path, self.loaded[self.schema_path])
            # 実際のschema検証を実行し、その呼出し回数だけを数える。
            with patch("validate_json.load_validated_companion", wraps=load_validated_companion) as loader:
                self.assertTrue(validate_one(pair, self.registry)[0])
                self.assertTrue(validate_one(pair, self.registry)[0])
                self.assertEqual(loader.call_count, 1)
                timestamp = companion.stat()
                document["monsters"][0]["id"] = 2
                updated = json.dumps(document)
                self.assertEqual(len(updated), len(original))
                companion.write_text(updated, encoding="utf-8")
                os.utime(companion, ns=(timestamp.st_atime_ns, timestamp.st_mtime_ns))
                ok, diagnostic = validate_one(pair, self.registry)
                self.assertFalse(ok, diagnostic)
                self.assertIn("unknown monster ID", diagnostic)
                self.assertEqual(loader.call_count, 2)
                message["groups"][0]["id_list"] = [2]
                target.write_text(json.dumps(message), encoding="utf-8")
                self.assertTrue(validate_one(pair, self.registry)[0])
                self.assertEqual(loader.call_count, 2)
                # 別のRegistryへ渡した結果を同じコンテキストの検証済みと扱わない。
                other_registry = build_schema_registry(self.loaded)
                self.assertTrue(validate_one(pair, other_registry)[0])
                self.assertEqual(loader.call_count, 3)
                companion.write_text("[]", encoding="utf-8")
                ok, diagnostic = validate_one(pair, other_registry)
                self.assertFalse(ok, diagnostic)
                self.assertIn("Invalid monster definitions", diagnostic)

    def test_monster_id_cache_rejects_float_definition_ids(self):
        with tempfile.TemporaryDirectory() as folder:
            directory = Path(folder)
            target = directory / "MonsterMessages.jsonc"
            target.write_text(json.dumps({"versions": 1, "groups": [{"id_list": [1], "message": []}]}), encoding="utf-8")
            document = {"versions": self.monrace_version, "monsters": [dict(self.monrace_template, id=1.0)]}
            (directory / "MonraceDefinitions.jsonc").write_text(json.dumps(document), encoding="utf-8")
            for _ in range(2):
                ok, diagnostic = validate_one((target, self.schema_path, self.loaded[self.schema_path]), self.registry)
                self.assertFalse(ok, diagnostic)
                self.assertIn("integer JSON value", diagnostic)
                self.assertIn("['monsters', 0, 'id']", diagnostic)

    def test_monster_id_cache_keeps_selected_source_when_adjacent_file_appears(self):
        with tempfile.TemporaryDirectory() as folder:
            directory = Path(folder)
            target = directory / "MonsterMessages.jsonc"
            target.write_text(json.dumps({"versions": 1, "groups": [{"id_list": [self.other_monrace_id], "message": []}]}), encoding="utf-8")
            pair = (target, self.schema_path, self.loaded[self.schema_path])
            registry = build_schema_registry(self.loaded)
            companion = {"versions": self.monrace_version, "monsters": [dict(self.monrace_template, id=1)]}

            def load_after_appearance(*args, **kwargs):
                (directory / "MonraceDefinitions.jsonc").write_text(json.dumps(companion), encoding="utf-8")
                return load_validated_companion(*args, **kwargs)

            # 選択済みの同梱表を読み、新たに現れた候補のIDでキャッシュを汚染しない。
            with patch("validate_json.load_validated_companion", side_effect=load_after_appearance):
                ok, diagnostic = validate_one(pair, registry)
                self.assertTrue(ok, diagnostic)
            with tempfile.TemporaryDirectory() as other_folder:
                other_target = Path(other_folder) / target.name
                other_target.write_bytes(target.read_bytes())
                ok, diagnostic = validate_one((other_target, self.schema_path, self.loaded[self.schema_path]), registry)
                self.assertTrue(ok, diagnostic)
            # 次の検証では隣接候補を優先し、候補に無いIDを拒否する。
            ok, diagnostic = validate_one(pair, registry)
            self.assertFalse(ok, diagnostic)
            self.assertIn("unknown monster ID", diagnostic)

    def test_monster_id_cache_observes_companion_schema_updates(self):
        with tempfile.TemporaryDirectory() as folder:
            directory = Path(folder)
            target = directory / "MonsterMessages.jsonc"
            companion = directory / "MonraceDefinitions.jsonc"
            schema_path = directory / "MonraceDefinitions.schema.json"
            schema = {"properties": {"monsters": {"items": {"properties": {"id": {"enum": [1]}}}}}}
            companion.write_text(json.dumps({"monsters": [{"id": 1}]}), encoding="utf-8")
            schema_path.write_text(json.dumps(schema), encoding="utf-8")
            data = {"groups": [{"id_list": [1], "message": []}]}
            validate_monster_message_semantics(data, directory / "MonsterMessages.schema.json", target, self.registry)
            schema["properties"]["monsters"]["items"]["properties"]["id"]["enum"] = [2]
            schema_path.write_text(json.dumps(schema), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Invalid monster definitions"):
                validate_monster_message_semantics(data, directory / "MonsterMessages.schema.json", target, self.registry)

    def message(self, languages=("ja", "en"), chance=1):
        return {"action": "SPEAK_ALL", "chance": chance, "message": {language: ["Hello"] for language in languages}}

    def validate(self, groups, *, companions=None):
        document = {"versions": 1, "groups": groups}
        return validate_document(document, self.schema_path, self.loaded[self.schema_path], self.registry, companions=companions)

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
        self.assert_valid([{"id_list": [1, self.max_monrace_id], "message": [self.message(chance=1), self.message(chance=100)]}])

    def test_monster_id_maximum_matches_monrace_id_storage(self):
        # モンスターIDの上限は、IDを格納する型(MonraceId)の上限−1とする。
        maximum = monrace_id_schema_maximum()
        id_list = self.loaded[self.schema_path]["properties"]["groups"]["items"]["properties"]["id_list"]
        self.assertEqual(id_list["items"]["maximum"], maximum)

    def test_undefined_monster_ids_are_rejected(self):
        for value in (self.max_monrace_id + 1, 32766):
            with self.subTest(value=value):
                self.assert_invalid([{"id_list": [1, value], "message": []}], ["groups", 0, "id_list", 1], "unknown monster ID")

    def test_float_id_is_rejected(self):
        self.assert_invalid([{"id_list": [1.0], "message": []}], ["groups", 0, "id_list", 0], "integer JSON value")

    def test_float_chance_is_rejected(self):
        self.assert_invalid([{"name": "DEFAULT", "message": [self.message(chance=1.0)]}],
                            ["groups", 0, "message", 0, "chance"], "integer JSON value")

    def test_other_invalid_ids_still_fail(self):
        for value in (True, "1", None, 1.5, 0, 32767):
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
        for ids in ([], [self.max_monrace_id]):
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
