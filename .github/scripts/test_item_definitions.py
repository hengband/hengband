"""ベースアイテム・アーティファクト定義の schema と意味検証が reader の受理範囲と一致することを確かめる。"""
import copy
import re
from pathlib import Path
import unittest
from jsonschema import ValidationError

from json_validation_test_helper import baseitem_id_schema_maximum, fixed_artifact_id_schema_maximum, validate_document
from validate_json import (build_schema_registry, load_all_schemas, load_cpp_enum_values, load_cpp_tokens, load_jsonc,
                           load_rod_type_value, validate_baseitem_semantics)


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

    def test_id_maximum_matches_baseitem_id_storage(self):
        # ベースアイテムIDの上限は、IDを扱う型(BaseitemList の short)の上限−1とする。
        maximum = baseitem_id_schema_maximum()
        schema = self.loaded[self.schema_map["BaseitemDefinitions"]]
        self.assertEqual(schema["properties"]["baseitems"]["items"]["properties"]["id"]["maximum"], maximum)

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
        self.check(self.changed(self.rod, ["parameter_value"], 0), False, ["baseitems", 0, "parameter_value"])
        record = copy.deepcopy(self.rod)
        del record["parameter_value"]
        self.check(record, False)
        self.check(self.changed(self.sample, ["parameter_value"], 0), True)

    def check_range_record(self, record, accepted):
        # schema と意味検証をそれぞれ通し、schema だけで拒否される場合も CI の規則自体を試す。
        self.check(record, accepted)
        schema_path = self.schema_map["BaseitemDefinitions"]
        document = {"baseitems": [record]}
        if accepted:
            validate_baseitem_semantics(document, schema_path)
        else:
            with self.assertRaises(ValidationError):
                validate_baseitem_semantics(document, schema_path)

    def range_record(self, kind):
        record = copy.deepcopy(self.sample)
        record["itemkind"] = {"type_value": kind, "subtype_value": 0}
        record["parameter_value"] = 1 if kind in (55, 65, 66, 77) else 0
        record["cost"] = 0
        return record

    def test_pval_boundaries_for_every_defined_kind(self):
        root = Path(__file__).resolve().parents[2]
        kinds = load_cpp_enum_values(root / "src/object/tval-types.h", "ItemKindType")
        equipment = {16, 17, 18, 19, 20, 21, 22, 23, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 45, 50}
        ranges = {55: (1, 127), 65: (1, 127), 66: (1, 1000), 80: (0, 10000),
                  75: (-10000, 10000), 77: (1, 15000), 7: (0, 63)}
        for kind in kinds.values():
            low, high = (-128, 128) if kind in equipment else ranges.get(kind, (0, 0))
            for value in (low - 1, low, high, high + 1):
                with self.subTest(kind=kind, value=value):
                    record = self.range_record(kind)
                    record["parameter_value"] = value
                    self.check_range_record(record, low <= value <= high)
            with self.subTest(kind=kind, omitted=True):
                record = self.range_record(kind)
                del record["parameter_value"]
                self.check_range_record(record, low <= 0 <= high)

    def test_only_defined_kinds_up_to_127_are_accepted(self):
        root = Path(__file__).resolve().parents[2]
        kinds = set(load_cpp_enum_values(root / "src/object/tval-types.h", "ItemKindType").values())
        for kind in range(-1, 130):
            with self.subTest(kind=kind):
                self.check_range_record(self.range_record(kind), kind in kinds and 0 <= kind <= 127)

    def test_reader_kind_table_matches_cpp_enum(self):
        root = Path(__file__).resolve().parents[2]
        kinds = load_cpp_enum_values(root / "src/object/tval-types.h", "ItemKindType")
        source = (root / "src/info-reader/baseitem-reader.cpp").read_text(encoding="utf-8")
        table = source.split("DEFINED_ITEM_KINDS[] = {", 1)[1].split("};", 1)[0]
        self.assertEqual(set(re.findall(r"ItemKindType::([A-Z_]+)", table)), set(kinds))

    def test_subtype_boundaries_per_kind(self):
        limits = {**dict.fromkeys((19, 20, 21, 22, 23), 63),
                  **dict.fromkeys((90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 105, 106, 107), 3),
                  55: 32, 65: 31, 66: 30, 16: 128, 30: 128, 1: 128}
        for kind, maximum in limits.items():
            for value in (-1, 0, maximum, maximum + 1):
                with self.subTest(kind=kind, value=value):
                    record = self.range_record(kind)
                    record["itemkind"]["subtype_value"] = value
                    self.check_range_record(record, 0 <= value <= maximum)

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

    def test_ac_and_bonuses_fit_ranges_whose_equipment_sums_do_not_overflow(self):
        # ベースACと修正値は絶対値が255以下の小さな数に限る (装備の合計の見積もりは item-reader-limits.h)。ベースACは負の値を認めない。
        for field, minimum in (("base_ac", 0), ("hit_bonus", -255), ("damage_bonus", -255), ("ac_bonus", -255)):
            with self.subTest(field=field):
                self.check(self.changed(self.sample, [field], minimum), True)
                self.check(self.changed(self.sample, [field], 255), True)
                self.check(self.changed(self.sample, [field], minimum - 1), False, ["baseitems", 0, field])
                self.check(self.changed(self.sample, [field], 256), False, ["baseitems", 0, field])

    def test_allocation_depth_fits_generation_table(self):
        # 出現階層は生成の表で要素数 MAX_DEPTH の配列の添え字になるため、BaseitemReader は MAX_DEPTH - 1 まで受け付ける。
        self.check(self.changed(self.sample, ["allocations"], [{"depth": 127, "rarity": 1}]), True)
        self.check(self.changed(self.sample, ["allocations"], [{"depth": 128, "rarity": 1}]), False, ["baseitems", 0, "allocations", 0, "depth"])
        self.check(self.changed(self.sample, ["allocations"], [{"depth": -1, "rarity": 1}]), False, ["baseitems", 0, "allocations", 0, "depth"])

    def test_natural_generation_false_excludes_allocations(self):
        # 自然生成しないベースアイテムは生成テーブルを持たないため、BaseitemReader は空でない allocations との併記を拒否する。
        without_allocations = copy.deepcopy(self.sample)
        del without_allocations["allocations"]
        for natural_generation in (True, False):
            with self.subTest(natural_generation=natural_generation):
                self.check({**without_allocations, "natural_generation": natural_generation}, True)
                self.check({**without_allocations, "natural_generation": natural_generation, "allocations": []}, True)
        self.check({**self.sample, "natural_generation": True}, True)
        self.check({**self.sample, "natural_generation": False}, False, ["baseitems", 0, "allocations"], "must not have allocations")
        for value in (0, 1, "false", []):
            with self.subTest(value=value):
                self.check({**without_allocations, "natural_generation": value}, False, ["baseitems", 0, "natural_generation"])

    def test_reader_tokens_and_rarity_boundary_are_accepted(self):
        for flag in ("ACTIVATE", "IM_LITE"):
            with self.subTest(flag=flag):
                self.check(self.changed(self.sample, ["flags"], [flag]), True)
        self.check(self.changed(self.sample, ["activate"], "DARKNESS"), True)
        # 出現率は 100÷希少度なので 101 以上はどれも 0 になり、0 は欄が無いのと同じになる。BaseitemReader は 1〜100 を受け付ける。
        for rarity, accepted in ((1, True), (100, True), (0, False), (101, False)):
            with self.subTest(rarity=rarity):
                record = self.changed(self.sample, ["allocations"], [{"depth": 1, "rarity": rarity}])
                self.check(record, accepted, None if accepted else ["baseitems", 0, "allocations", 0, "rarity"])

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

    def test_ac_and_bonuses_fit_ranges_whose_equipment_sums_do_not_overflow(self):
        # ベースACと修正値は絶対値が255以下の小さな数に限る (装備の合計の見積もりは item-reader-limits.h)。ベースACは負の値を認めない。
        for field, minimum in (("base_ac", 0), ("hit_bonus", -255), ("damage_bonus", -255), ("ac_bonus", -255)):
            with self.subTest(field=field):
                self.check({**self.sample, field: minimum}, True)
                self.check({**self.sample, field: 255}, True)
                self.check({**self.sample, field: minimum - 1}, False, ["artifacts", 0, field])
                self.check({**self.sample, field: 256}, False, ["artifacts", 0, field])

    def test_id_maximum_matches_fixed_artifact_id_storage(self):
        # 固定アーティファクトIDの上限は、IDを格納する型(FixedArtifactId)の上限−1とする。
        maximum = fixed_artifact_id_schema_maximum()
        schema = self.loaded[self.schema_map["ArtifactDefinitions"]]
        self.assertEqual(schema["properties"]["artifacts"]["items"]["properties"]["id"]["maximum"], maximum)
        self.check({**self.sample, "id": maximum + 1}, False, ["artifacts", 0, "id"])

    def test_ids_must_start_at_one_without_gaps(self):
        # 実行時の記録は 1〜件数の ID だけを作るため、0・欠番・重複・逆順を拒否する。
        for first_id in (2, 32766):
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
        other = {**baseitem, "id": 1, "itemkind": {"type_value": 127, "subtype_value": 0}, "parameter_value": 0}
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


class DefinitionLevelLimitTest(unittest.TestCase):
    def test_level_is_limited_to_the_deepest_reachable_floor(self):
        # ベースアイテム・アーティファクトの生成階と、同じ発動の難易度の計算に使うエゴの level は、MAX_DEPTH - 1 までとする。
        root = Path(__file__).resolve().parents[2]
        header = (root / "src/floor/floor-base-definitions.h").read_text(encoding="utf-8")
        match = re.search(r"constexpr int MAX_DEPTH = (\d+);", header)
        self.assertIsNotNone(match, "MAX_DEPTH not found in floor-base-definitions.h")
        max_depth = int(match.group(1))
        loaded, schema_map = load_all_schemas(root / "schema")
        levels = {
            "BaseitemDefinitions": loaded[schema_map["BaseitemDefinitions"]]["properties"]["baseitems"]["items"]["properties"]["level"],
            "ArtifactDefinitions": loaded[schema_map["ArtifactDefinitions"]]["properties"]["artifacts"]["items"]["properties"]["level"],
        }
        levels["EgoDefinitions"] = loaded[schema_map["EgoDefinitions"]]["definitions"]["ego"]["properties"]["level"]
        for name, level in levels.items():
            with self.subTest(name=name):
                self.assertEqual((level["minimum"], level["maximum"]), (0, max_depth - 1))

    def test_level_boundaries(self):
        root = Path(__file__).resolve().parents[2]
        loaded, schema_map = load_all_schemas(root / "schema")
        registry = build_schema_registry(loaded)
        for name, key in (("BaseitemDefinitions", "baseitems"), ("ArtifactDefinitions", "artifacts")):
            document = load_jsonc(root / "lib/edit" / f"{name}.jsonc")
            sample = {**document[key][0], "id": 1}
            schema_path = schema_map[name]
            for level, accepted in ((127, True), (128, False)):
                with self.subTest(name=name, level=level):
                    ok, message = validate_document({"version": document["version"], key: [{**sample, "level": level}]},
                                                    schema_path, loaded[schema_path], registry)
                    self.assertEqual(ok, accepted, message)
                    if not accepted:
                        self.assertIn(f"Location: {[key, 0, 'level']}", message)


class ItemBonusLimitTest(unittest.TestCase):
    def test_schema_matches_reader_limits(self):
        # schema の範囲が item-reader-limits.h の定数と合っていることを確かめる。
        root = Path(__file__).resolve().parents[2]
        header = (root / "src/info-reader/item-reader-limits.h").read_text(encoding="utf-8")
        limits = {name: int(value) for name, value in re.findall(r"constexpr int (ITEM_BASE_AC_MAX|ITEM_BONUS_MAX) = (\d+);", header)}
        self.assertEqual(set(limits), {"ITEM_BASE_AC_MAX", "ITEM_BONUS_MAX"}, "ITEM_BASE_AC_MAX / ITEM_BONUS_MAX not found in item-reader-limits.h")
        loaded, schema_map = load_all_schemas(root / "schema")
        for name, key in (("BaseitemDefinitions", "baseitems"), ("ArtifactDefinitions", "artifacts")):
            properties = loaded[schema_map[name]]["properties"][key]["items"]["properties"]
            with self.subTest(name=name):
                self.assertEqual((properties["base_ac"]["minimum"], properties["base_ac"]["maximum"]), (0, limits["ITEM_BASE_AC_MAX"]))
                for field in ("hit_bonus", "damage_bonus", "ac_bonus"):
                    self.assertEqual((properties[field]["minimum"], properties[field]["maximum"]), (-limits["ITEM_BONUS_MAX"], limits["ITEM_BONUS_MAX"]))


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
