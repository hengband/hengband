"""ベースアイテム・アーティファクト定義の schema と意味検証が reader の受理範囲と一致することを確かめる。"""
import copy
from pathlib import Path
import unittest

from json_validation_test_helper import validate_document
from validate_json import build_schema_registry, load_all_schemas, load_cpp_tokens, load_jsonc, load_rod_type_value


class BaseitemDefinitionValidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[2]
        cls.loaded, cls.schema_map = load_all_schemas(root / "schema")
        cls.registry = build_schema_registry(cls.loaded)
        cls.document = load_jsonc(root / "lib/edit/BaseitemDefinitions.jsonc")
        baseitems = cls.document["baseitems"]
        cls.sample = next(item for item in baseitems if {"parameter_value", "base_dice", "allocations"} <= item.keys())
        rod_type_value = load_rod_type_value(root)
        cls.rod = next(item for item in baseitems if item["itemkind"]["type_value"] == rod_type_value)

    def validate(self, *baseitems):
        schema_path = self.schema_map["BaseitemDefinitions"]
        document = {"version": self.document["version"], "baseitems": list(baseitems)}
        return validate_document(document, schema_path, self.loaded[schema_path], self.registry)

    def check(self, record, accepted, location=None, reason=None):
        ok, message = self.validate(record)
        self.assertEqual(ok, accepted, message)
        if location is not None:
            self.assertIn(f"Location: {location}", message)
        if reason is not None:
            self.assertIn(reason, message)

    def changed(self, base, path, value):
        record = copy.deepcopy(base)
        target = record
        for part in path[:-1]:
            target = target[part]
        target[path[-1]] = value
        return record

    def test_bundled_definitions(self):
        ok, message = self.validate(*self.document["baseitems"])
        self.assertTrue(ok, message)

    def test_integer_representation_matches_reader(self):
        paths = [[field] for field in ("id", "parameter_value", "level", "weight", "cost")]
        paths += [["itemkind", "type_value"], ["itemkind", "subtype_value"], ["allocations", 0, "depth"], ["allocations", 0, "rarity"]]
        for path in paths:
            with self.subTest(path=path):
                value = copy.deepcopy(self.sample)
                for part in path[:-1]:
                    value = value[part]
                record = self.changed(self.sample, path, float(value[path[-1]]))
                self.check(record, False, ["baseitems", 0] + path, "integer JSON value")
        for field in ("base_ac", "hit_bonus", "damage_bonus", "ac_bonus"):
            with self.subTest(field=field):
                self.check(self.changed(self.sample, [field], 1.0), False, ["baseitems", 0, field], "integer JSON value")

    def test_ids_must_be_strictly_increasing(self):
        for second_id in (self.sample["id"], self.sample["id"] - 1):
            with self.subTest(second_id=second_id):
                ok, message = self.validate(self.sample, {**self.sample, "id": second_id})
                self.assertFalse(ok, message)
                self.assertIn("Location: ['baseitems', 1, 'id']", message)
        # 種別・副種別の重複は別の規則で拒否するため、2件目は副種別を変える。
        later = self.changed({**self.sample, "id": self.sample["id"] + 2}, ["itemkind", "subtype_value"], self.sample["itemkind"]["subtype_value"] + 1)
        ok, message = self.validate(self.sample, later)
        self.assertTrue(ok, message)

    def test_symbol_and_level_are_required(self):
        for field in ("symbol", "level"):
            with self.subTest(field=field):
                record = copy.deepcopy(self.sample)
                del record[field]
                self.check(record, False, reason=f"'{field}' is a required property")

    def test_symbol_matches_reader(self):
        for character, accepted in (("\\", True), ("~", True), ("", False), ("ab", False), ("é", False)):
            with self.subTest(character=character):
                record = self.changed(self.sample, ["symbol", "character"], character)
                self.check(record, accepted, None if accepted else ["baseitems", 0, "symbol", "character"])
        for color, accepted in (("Light Brown", True), ("Black", True), ("Brown ", False), ("Pink", False)):
            with self.subTest(color=color):
                record = self.changed(self.sample, ["symbol", "color"], color)
                self.check(record, accepted, None if accepted else ["baseitems", 0, "symbol", "color"])

    def test_rod_parameter_value_must_be_positive(self):
        self.check(self.changed(self.rod, ["parameter_value"], 1), True)
        self.check(self.changed(self.rod, ["parameter_value"], 0), False, ["baseitems", 0, "parameter_value"], "must be positive")
        record = copy.deepcopy(self.rod)
        del record["parameter_value"]
        self.check(record, False, ["baseitems", 0, "parameter_value"], "must be positive")
        self.check(self.changed(self.sample, ["parameter_value"], 0), True)

    def test_dice_matches_reader(self):
        # from_chars は先頭の0をいくつでも読み飛ばす。セーブでは個数・面数を1バイトで書くため0〜255に限る。
        for dice in ("1d4", "0d0", "007d08", "255d255", "0" * 5000 + "1d4"):
            with self.subTest(dice=dice[-8:]):
                self.check(self.changed(self.sample, ["base_dice"], dice), True)
        for dice in ("d", "1d", "d4", "1d4\n", "1d2d3", "+1d4", "1D4", " 1d4", "xd5y", "2147483648d1", "1d-2147483649",
                     "1" + "0" * 5000 + "d4", "-1d2", "1d-1", "256d1", "1d256", "2147483647d1"):
            with self.subTest(dice=dice[:16]):
                self.check(self.changed(self.sample, ["base_dice"], dice), False, ["baseitems", 0, "base_dice"])

    def test_allocations_fit_reader_tables(self):
        allocation = {"depth": 1, "rarity": 1}
        self.check(self.changed(self.sample, ["allocations"], [allocation] * 4), True)
        self.check(self.changed(self.sample, ["allocations"], [allocation] * 5), False, ["baseitems", 0, "allocations"])
        self.check(self.changed(self.sample, ["allocations"], [1]), False, ["baseitems", 0, "allocations", 0])

    def test_allocation_depth_fits_generation_table(self):
        # 出現階層は生成の表で要素数 MAX_DEPTH の配列の添え字になるため、BaseitemReader は MAX_DEPTH - 1 まで受け付ける。
        self.check(self.changed(self.sample, ["allocations"], [{"depth": 127, "rarity": 1}]), True)
        self.check(self.changed(self.sample, ["allocations"], [{"depth": 128, "rarity": 1}]), False, ["baseitems", 0, "allocations", 0, "depth"])
        self.check(self.changed(self.sample, ["allocations"], [{"depth": -1, "rarity": 1}]), False, ["baseitems", 0, "allocations", 0, "depth"])

    def test_reader_tokens_and_rarity_boundary_are_accepted(self):
        for flag in ("ACTIVATE", "IM_LITE"):
            with self.subTest(flag=flag):
                self.check(self.changed(self.sample, ["flags"], [flag]), True)
        self.check(self.changed(self.sample, ["activate"], "DARKNESS"), True)
        # 出現率は 100÷希少度なので 101 以上はどれも 0 になるが、BaseitemReader は 256 まで受け付ける。
        self.check(self.changed(self.sample, ["allocations"], [{"depth": 1, "rarity": 256}]), True)
        self.check(self.changed(self.sample, ["allocations"], [{"depth": 1, "rarity": 257}]), False, ["baseitems", 0, "allocations", 0, "rarity"])

    def test_kinds_of_valid_baseitems_must_be_unique(self):
        # 種別・副種別から ID を引く表では後の ID が先の ID を隠すため、有効なベースアイテムどうしの重複を拒否する。
        second = {**self.sample, "id": self.sample["id"] + 1}
        ok, message = self.validate(self.sample, second)
        self.assertFalse(ok, message)
        self.assertIn("Location: ['baseitems', 1, 'itemkind']", message)
        other = self.changed(second, ["itemkind", "subtype_value"], self.sample["itemkind"]["subtype_value"] + 1)
        ok, message = self.validate(self.sample, other)
        self.assertTrue(ok, message)
        # 名前が空か「異常アイテム」のベースアイテムは、その言語のビルドで表に載らないので重なってもよい。
        for names in ({"ja": "異常アイテム", "en": "Invalid item"}, {"ja": "", "en": ""}):
            with self.subTest(names=names):
                ok, message = self.validate(self.sample, {**second, "name": names})
                self.assertTrue(ok, message)
        # 片方の言語だけ無効なら、もう片方の言語のビルドでは重なる。
        for language, invalid in (("ja", "異常アイテム"), ("en", "Invalid item")):
            with self.subTest(language=language):
                ok, message = self.validate(self.sample, {**second, "name": {**second["name"], language: invalid}})
                self.assertFalse(ok, message)
                self.assertIn("Location: ['baseitems', 1, 'itemkind']", message)


class ArtifactDefinitionValidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[2]
        cls.loaded, cls.schema_map = load_all_schemas(root / "schema")
        cls.registry = build_schema_registry(cls.loaded)
        cls.document = load_jsonc(root / "lib/edit/ArtifactDefinitions.jsonc")
        sample = next(artifact for artifact in cls.document["artifacts"]
                      if {"parameter_value", "base_ac", "base_dice", "hit_bonus", "damage_bonus", "ac_bonus"} <= artifact.keys())
        # 単独の定義として検証するため、ID は先頭の 1 にする。
        cls.sample = {**sample, "id": 1}
        cls.monraces = load_jsonc(root / "lib/edit/MonraceDefinitions.jsonc")
        cls.baseitem_document = load_jsonc(root / "lib/edit/BaseitemDefinitions.jsonc")

    def validate(self, *artifacts):
        schema_path = self.schema_map["ArtifactDefinitions"]
        document = {"version": self.document["version"], "artifacts": list(artifacts)}
        return validate_document(document, schema_path, self.loaded[schema_path], self.registry)

    def check(self, record, accepted, location=None, reason=None):
        ok, message = self.validate(record)
        self.assertEqual(ok, accepted, message)
        if location is not None:
            self.assertIn(f"Location: {location}", message)
        if reason is not None:
            self.assertIn(reason, message)

    def test_bundled_definitions(self):
        ok, message = self.validate(*self.document["artifacts"])
        self.assertTrue(ok, message)

    def test_integer_representation_matches_reader(self):
        for field in ("id", "parameter_value", "level", "rarity", "weight", "cost", "base_ac", "hit_bonus", "damage_bonus", "ac_bonus"):
            with self.subTest(field=field):
                self.check({**self.sample, field: float(self.sample[field])}, False, ["artifacts", 0, field], "integer JSON value")
        for field in ("type_value", "subtype_value"):
            with self.subTest(field=field):
                base_item = {**self.sample["base_item"], field: float(self.sample["base_item"][field])}
                self.check({**self.sample, "base_item": base_item}, False, ["artifacts", 0, "base_item", field], "integer JSON value")

    def test_ids_must_start_at_one_without_gaps(self):
        # 実行時の記録は 1〜件数の ID だけを作るため、0・欠番・重複・逆順を拒否する。
        for first_id in (2, 9999):
            with self.subTest(first_id=first_id):
                self.check({**self.sample, "id": first_id}, False, ["artifacts", 0, "id"], "start at 1")
        self.check({**self.sample, "id": 0}, False, ["artifacts", 0, "id"])
        for second_id in (1, 3, 0):
            with self.subTest(second_id=second_id):
                ok, message = self.validate(self.sample, {**self.sample, "id": second_id})
                self.assertFalse(ok, message)
                self.assertIn("Location: ['artifacts', 1, 'id']", message)
        ok, message = self.validate(self.sample, {**self.sample, "id": 2})
        self.assertTrue(ok, message)

    def test_dice_matches_reader(self):
        for dice in ("1d4", "0d0", "255d255", "0" * 5000 + "1d4"):
            with self.subTest(dice=dice[-8:]):
                self.check({**self.sample, "base_dice": dice}, True)
        for dice in ("d", "1d", "1d4\n", "1d2d3", "+1d4", "2147483648d1", "-1d2", "1d-1", "256d1", "1d256"):
            with self.subTest(dice=dice):
                self.check({**self.sample, "base_dice": dice}, False, ["artifacts", 0, "base_dice"])

    def validate_with_baseitems(self, artifact, baseitems):
        schema_path = self.schema_map["ArtifactDefinitions"]
        document = {"version": self.document["version"], "artifacts": [artifact]}
        companion = {"version": self.baseitem_document["version"], "baseitems": baseitems}
        return validate_document(document, schema_path, self.loaded[schema_path], self.registry,
                                 companions={"BaseitemDefinitions.jsonc": companion})

    def test_base_item_must_be_a_valid_baseitem(self):
        # 生成・鑑定では元のベースアイテムを種別・副種別から引き、無ければ例外で落ちる。
        for field, value in (("type_value", 128), ("subtype_value", 128)):
            with self.subTest(field=field):
                base_item = {**self.sample["base_item"], field: value}
                self.check({**self.sample, "base_item": base_item}, False, ["artifacts", 0, "base_item"], "Reason: base item must be a valid baseitem")

        key = self.sample["base_item"]
        baseitem = next(item for item in self.baseitem_document["baseitems"]
                        if (item["itemkind"]["type_value"], item["itemkind"]["subtype_value"]) == (key["type_value"], key["subtype_value"]))
        ok, message = self.validate_with_baseitems(self.sample, [{**baseitem, "id": 1}])
        self.assertTrue(ok, message)
        # 隣の候補のベースアイテム定義を優先し、同梱定義にあっても候補に無ければ拒否する。
        other = {**baseitem, "id": 1, "itemkind": {"type_value": 127, "subtype_value": 0}}
        ok, message = self.validate_with_baseitems(self.sample, [other])
        self.assertFalse(ok, message)
        self.assertIn("Reason: base item must be a valid baseitem", message)
        # どちらかの言語の名前が無効なベースアイテムは、そのビルドで実在しない扱いになる。
        for language, invalid in (("ja", "異常アイテム"), ("en", "Invalid item"), ("ja", ""), ("en", "")):
            with self.subTest(language=language, name=invalid):
                renamed = {**baseitem, "id": 1, "name": {**baseitem["name"], language: invalid}}
                ok, message = self.validate_with_baseitems(self.sample, [renamed])
                self.assertFalse(ok, message)
                self.assertIn("Reason: base item must be a valid baseitem", message)
        # 同じ種別の別の項目が、それぞれの言語で有効なら、どちらのビルドでも実在する。
        japanese_only = {**baseitem, "id": 1, "name": {**baseitem["name"], "en": "Invalid item"}}
        english_only = {**baseitem, "id": 2, "name": {**baseitem["name"], "ja": "異常アイテム"}}
        ok, message = self.validate_with_baseitems(self.sample, [japanese_only, english_only])
        self.assertTrue(ok, message)

    def test_monrace_validation_checks_artifact_companion(self):
        # モンスター定義の検証は、参照先のアーティファクト定義にも同じ意味検証をかける。
        schema_path = self.schema_map["MonraceDefinitions"]
        companion = {"version": self.document["version"], "artifacts": [{**self.sample, "level": float(self.sample["level"])}]}
        ok, message = validate_document(self.monraces, schema_path, self.loaded[schema_path], self.registry,
                                        companions={"ArtifactDefinitions.jsonc": companion})
        self.assertFalse(ok, message)
        self.assertIn("Invalid artifact definitions", message)
        self.assertIn("integer JSON value", message)


class ItemTokenListTest(unittest.TestCase):
    def test_schema_token_lists_match_reader_tables(self):
        # フラグと発動の一覧は、BaseitemReader・ArtifactReader が使う C++ の表と同じにする。
        root = Path(__file__).resolve().parents[2]
        flags = load_cpp_tokens(root / "src/info-reader/baseitem-tokens-table.cpp")
        activations = load_cpp_tokens(root / "src/object-enchant/activation-info-table.cpp")
        for name, array in (("BaseitemDefinitions", "baseitems"), ("ArtifactDefinitions", "artifacts")):
            with self.subTest(name=name):
                item = load_jsonc(root / "schema" / f"{name}.schema.json")["properties"][array]["items"]["properties"]
                self.assertEqual(set(item["flags"]["items"]["enum"]), flags)
                self.assertEqual(set(item["activate"]["enum"]), activations)


if __name__ == "__main__":
    unittest.main()
