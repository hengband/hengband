"""クエスト定義の CI の検証が QuestReader・QuestList と一致することを確かめる。"""
import copy
import json
from pathlib import Path
import tempfile
import unittest

from json_validation_test_helper import (dungeon_id_maximum, fixed_artifact_id_schema_maximum, monrace_id_schema_maximum,
                                         quest_id_schema_maximum, validate_document)
from validate_json import MAX_EGO_ID, build_schema_registry, load_all_schemas, load_jsonc, validate_one


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

    def assert_schema_accepted(self, ok, message):
        # 上限の ID は同梱の定義に無いため、schema を通った後の参照の照合で拒否されてよい。
        self.assertTrue(ok or "Reason: unknown" in message, message)

    def assert_boundary(self, make, accepted, rejected, location=None):
        ok, message = self.validate("Quest", make(accepted))
        self.assert_schema_accepted(ok, message)
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

    def test_legend_ego_follows_ego_id_range(self):
        # エゴのIDはエゴ定義でもセーブでも1バイトに収まる 1〜255 に限られる。
        shared = self.loaded[next(path for path in self.loaded if path.name == "QuestGrid.schema.json")]
        plain, random = shared["$defs"]["egoSpec"]["oneOf"]
        self.assertEqual((plain["minimum"], plain["maximum"]), (1, MAX_EGO_ID))
        self.assertNotIn("id", random["properties"])
        cases = (
            lambda value: {"ego": value},
        )
        for make_cell in cases:
            for schema_name in ("Quest", "QuestPreferences"):
                for value in (1, 255, 0, 256, -1):
                    with self.subTest(schema=schema_name, cell=make_cell(value)):
                        document = self.quest(make_cell(value)) if schema_name == "Quest" else {"version": 1, "legend": {"#": make_cell(value)}}
                        ok, message = self.validate(schema_name, document)
                        if value in (1, 255):
                            self.assert_schema_accepted(ok, message)
                        else:
                            self.assertFalse(ok, message)
                            reason = "256 is greater than the maximum of 255" if value == 256 else f"{value} is less than the minimum of 1"
                            self.assertIn(reason, message)

    def test_special_follows_reader_signed_int16_range(self):
        for accepted, rejected in ((32767, 32768), (-32768, -32769)):
            with self.subTest(rejected=rejected):
                self.assert_boundary(lambda value: self.quest({"special": value}), accepted, rejected, ["legend", "#", "special"])

    def test_definition_ids_follow_cpp_id_maximums(self):
        # 対象モンスターは MonraceId、報酬のアーティファクトは FixedArtifactId の基になる型の上限−1までとする。
        monster_maximum = monrace_id_schema_maximum()
        artifact_maximum = fixed_artifact_id_schema_maximum()
        schema = self.loaded[self.schema_map["Quest"]]
        reward = schema["$defs"]["reward"]["properties"]
        self.assertEqual(schema["$defs"]["definition"]["properties"]["monster"]["maximum"], monster_maximum)
        self.assertEqual(reward["artifact"]["maximum"], artifact_maximum)
        self.assertEqual(reward["artifacts"]["items"]["maximum"], artifact_maximum)
        cases = (
            (lambda value: {"monster": value}, monster_maximum, ["definition", "monster"]),
            (lambda value: {"reward": {"artifact": value}}, artifact_maximum, ["definition", "reward", "artifact"]),
            (lambda value: {"reward": {"artifacts": [1, value]}}, artifact_maximum, ["definition", "reward", "artifacts", 1]),
        )
        for make_definition, maximum, location in cases:
            with self.subTest(location=location):
                ok, message = self.validate("Quest", self.quest(definition=make_definition(maximum)))
                self.assert_schema_accepted(ok, message)
                ok, message = self.validate("Quest", self.quest(definition=make_definition(maximum + 1)))
                self.assertFalse(ok, message)
                self.assertIn(f"{maximum + 1} is greater than the maximum of {maximum}", message)
                self.assertIn(f"Location: {location}", message)

    def test_level_and_kill_count_follow_reader_ranges(self):
        # 階層は MAX_DEPTH - 1 まで、撃破数はセーブの16ビットまでしか読み込まない。
        for field, maximum in (("level", 127), ("maxNum", 32767)):
            for value, accepted in ((0, True), (maximum, True), (maximum + 1, False), (-1, False)):
                with self.subTest(field=field, value=value):
                    ok, message = self.validate("Quest", self.quest(definition={field: value}))
                    self.assertEqual(ok, accepted, message)
                    if not accepted:
                        self.assertIn(f"Location: {['definition', field]}", message)

    def test_dungeon_and_leaving_quest_maximums_match_cpp(self):
        # ダンジョンIDは DungeonId::MAX − 1、離脱するクエストのIDは QuestId の型の上限−1までとする。
        schema = self.loaded[self.schema_map["Quest"]]
        self.assertEqual(schema["$defs"]["definition"]["properties"]["dungeon"]["maximum"], dungeon_id_maximum())
        self.assertEqual(schema["$defs"]["startVariant"]["properties"]["leavingQuest"]["maximum"], quest_id_schema_maximum())

    def test_other_integers_follow_reader_ranges(self):
        # QuestReader は int に収まらない値を拒否し、開始位置は地図（最大 66 行 198 列）の中に限る。
        # ダンジョンIDと離脱するクエストのIDは、型と定数から決めた上限までに限る。
        int_max = 2147483647
        full_map = ["#" * 198] * 66
        cases = (
            ("numMon", int_max, lambda value: self.quest(definition={"numMon": value}), ["definition", "numMon"]),
            ("dungeon", dungeon_id_maximum(), lambda value: self.quest(definition={"dungeon": value}), ["definition", "dungeon"]),
            ("start.y", 65, lambda value: {**self.quest(), "map": full_map, "start": {"y": value, "x": 0}}, ["start", "y"]),
            ("start.x", 197, lambda value: {**self.quest(), "map": full_map, "start": {"y": 0, "x": value}}, ["start", "x"]),
            ("leavingQuest", quest_id_schema_maximum(),
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
                self.assert_schema_accepted(ok, message)
                ok, message = self.validate("Quest", make(maximum + 1))
                self.assertFalse(ok, message)
                self.assertIn(f"{maximum + 1} is greater than the maximum of {maximum}", message)
                self.assertIn(f"Location: {location}", message)

    def test_quest_preferences_share_the_legend_ranges(self):
        document = {"version": 1, "legend": {"#": {"monster": 32767, "special": -32768}}}
        ok, message = self.validate("QuestPreferences", document)
        self.assert_schema_accepted(ok, message)
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


class QuestEditDirTestCase(unittest.TestCase):
    """lib/edit と同じ形の一時ディレクトリでクエストを検証するテストの共通処理。"""

    @classmethod
    def setUpClass(cls):
        cls.repository = Path(__file__).resolve().parents[2]
        cls.loaded, cls.schema_map = load_all_schemas(cls.repository / "schema")
        cls.registry = build_schema_registry(cls.loaded)

    def quest(self, quest_id=1, **fields):
        document = {
            "version": 1,
            "id": quest_id,
            "name": {"ja": "試験", "en": "Test"},
            "definition": {"type": "KILL_ALL", "level": 1, "monster": 1, "dungeon": 1, "reward": {"artifact": 1}},
            "legend": {
                "#": {"terrain": "FLOOR", "trap": "NONE", "monster": 1, "object": 1, "artifact": 1},
                "c": {"monster": {"cloneOf": 1}, "artifact": {"random": True, "id": 1}},
                "r": {"terrain": "*", "trap": {"random": True}, "monster": {"random": True, "oodLevel": 32767},
                      "object": {"random": True, "oodLevel": 32767}, "artifact": {"questReward": True}},
            },
            "map": ["#cr"],
        }
        document.update(fields)
        return document

    def validate_in_edit_dir(self, quests, target, edit_files=None, schema_name="Quest", raw_quests=None):
        """lib/edit と同じ形の一時ディレクトリ（クエストは quests/ の下）に置いて検証する。"""
        with tempfile.TemporaryDirectory() as folder:
            edit_dir = Path(folder)
            (edit_dir / "quests").mkdir()
            for name, document in quests.items():
                (edit_dir / "quests" / name).write_text(json.dumps(document), encoding="utf-8")
            for name, text in (raw_quests or {}).items():
                (edit_dir / "quests" / name).write_text(text, encoding="utf-8")
            for name, document in (edit_files or {}).items():
                (edit_dir / name).write_text(json.dumps(document), encoding="utf-8")
            schema_path = self.schema_map[schema_name]
            data_path = edit_dir / target if schema_name == "QuestPreferences" else edit_dir / "quests" / target
            return validate_one((data_path, schema_path, self.loaded[schema_path]), self.registry)

    def bundled(self, name):
        return load_jsonc(self.repository / "lib/edit" / f"{name}.jsonc")

    def assert_rejected(self, result, reason, location):
        ok, message = result
        self.assertFalse(ok, message)
        self.assertIn(f"Reason: {reason}", message)
        self.assertIn(f"Location: {location}", message)


class QuestReferenceTest(QuestEditDirTestCase):
    """CI が QuestList と同じくクエストの参照を照合することを確かめる。"""

    def test_legend_ego_validation_in_local_and_shared_legends(self):
        cases = [
            ({"object": 42, "ego": 65}, True),
            ({"object": 42, "ego": {"random": True}}, True),
            ({"object": 42, "ego": 1}, False),
            ({"object": 42, "ego": 255}, False),
            ({"object": 42, "ego": 102}, False),
            ({"ego": 65}, False),
            ({"object": {"random": True}, "ego": 65}, False),
            ({"object": {"questReward": True}, "ego": 65}, False),
            ({"object": 42, "ego": 65, "trap": "CLOSED_DOOR"}, False),
            ({"object": 42, "ego": {"random": True}, "trap": {"random": True}}, False),
        ]
        gold = next(item["id"] for item in self.bundled("BaseitemDefinitions")["baseitems"]
                    if item["itemkind"]["type_value"] == 127)
        cases.append(({"object": gold, "ego": 65}, False))
        for cell, accepted in cases:
            for shared in (False, True):
                with self.subTest(cell=cell, shared=shared):
                    document = self.quest()
                    document["legend"]["#"] = cell
                    extra = {"QuestPreferences.jsonc": {"version": 1, "legend": {"#": cell}}}
                    schema = "QuestPreferences" if shared else "Quest"
                    target = "QuestPreferences.jsonc" if shared else "001.jsonc"
                    ok, message = self.validate_in_edit_dir({"001.jsonc": document}, target, extra, schema)
                    self.assertEqual(ok, accepted, message)

    def test_defined_references_are_accepted(self):
        ok, message = self.validate_in_edit_dir({"001.jsonc": self.quest()}, "001.jsonc")
        self.assertTrue(ok, message)

    def test_undefined_references_are_rejected(self):
        def mutate(change):
            document = self.quest()
            change(document)
            return document

        cases = (
            (lambda d: d["definition"].update(monster=32766), "unknown monster ID", ["definition", "monster"]),
            (lambda d: d["definition"].update(reward={"artifact": 32766}), "unknown artifact ID", ["definition", "reward", "artifact"]),
            (lambda d: d["definition"].update(reward={"artifacts": [1, 32766]}), "unknown artifact ID",
             ["definition", "reward", "artifacts", 1]),
            (lambda d: d["legend"]["#"].update(terrain="NO_SUCH_TERRAIN"), "unknown terrain tag", ["legend", "#", "terrain"]),
            (lambda d: d["legend"]["#"].update(trap="NO_SUCH_TERRAIN"), "unknown terrain tag", ["legend", "#", "trap"]),
            (lambda d: d["legend"]["#"].update(monster=32767), "unknown monster ID", ["legend", "#", "monster"]),
            (lambda d: d["legend"]["c"].update(monster={"cloneOf": 32767}), "unknown monster ID", ["legend", "c", "monster", "cloneOf"]),
            (lambda d: d["legend"]["#"].update(object=32767), "unknown or invalid baseitem ID", ["legend", "#", "object"]),
            (lambda d: d["legend"]["#"].update(artifact=32767), "unknown artifact ID", ["legend", "#", "artifact"]),
            # ランダムなアーティファクトの id も、配置では固定のアーティファクトとして使われる。
            (lambda d: d["legend"]["c"].update(artifact={"random": True, "id": 32767}), "unknown artifact ID",
             ["legend", "c", "artifact", "id"]),
        )
        for change, reason, location in cases:
            with self.subTest(location=location):
                self.assert_rejected(self.validate_in_edit_dir({"001.jsonc": mutate(change)}, "001.jsonc"), reason, location)

    def test_undefined_dungeon_is_rejected(self):
        # 範囲内でも、ダンジョン定義に無いIDは参照の照合で拒否する。
        document = self.quest()
        document["definition"]["dungeon"] = dungeon_id_maximum()
        dungeons = self.bundled("DungeonDefinitions")
        dungeons["dungeons"] = [dungeon for dungeon in dungeons["dungeons"] if dungeon["id"] != dungeon_id_maximum()]
        result = self.validate_in_edit_dir({"001.jsonc": document}, "001.jsonc", {"DungeonDefinitions.jsonc": dungeons})
        self.assert_rejected(result, "unknown dungeon ID", ["definition", "dungeon"])

    def test_omitted_monster_and_zero_dungeon_follow_the_reader(self):
        # 対象モンスターは省略すると 0 として照合され、dungeon の 0 は指定なしとして照合しない。
        document = self.quest()
        del document["definition"]["monster"]
        document["definition"]["dungeon"] = 0
        ok, message = self.validate_in_edit_dir({"001.jsonc": document}, "001.jsonc")
        self.assertTrue(ok, message)
        # 省略した対象モンスターも、ID 0 が定義に無ければ拒否する。
        monsters = self.bundled("MonraceDefinitions")
        monsters["monsters"] = [monster for monster in monsters["monsters"] if monster["id"] != 0]
        result = self.validate_in_edit_dir({"001.jsonc": document}, "001.jsonc", {"MonraceDefinitions.jsonc": monsters})
        self.assert_rejected(result, "unknown monster ID", ["definition", "monster"])

    def test_leaving_quest_must_name_a_quest_in_the_directory(self):
        quests = {"002.jsonc": self.quest(2)}
        for leaving, accepted in ((2, True), (1, True), (0, True), (3, False)):
            with self.subTest(leaving=leaving):
                quests["001.jsonc"] = self.quest(startVariants=[{"y": 0, "x": 0}, {"leavingQuest": leaving, "y": 0, "x": 1}])
                result = self.validate_in_edit_dir(quests, "001.jsonc")
                if accepted:
                    self.assertTrue(result[0], result[1])
                else:
                    self.assert_rejected(result, "unknown quest ID", ["startVariants", 1, "leavingQuest"])
        # start があれば startVariants は読まれないため、照合しない。
        quests["001.jsonc"] = self.quest(start={"y": 0, "x": 0}, startVariants=[{"leavingQuest": 3, "y": 0, "x": 0}])
        ok, message = self.validate_in_edit_dir(quests, "001.jsonc")
        self.assertTrue(ok, message)

    def test_baseitem_must_be_valid_in_both_languages(self):
        for language, name in (("ja", ""), ("ja", "異常アイテム"), ("en", ""), ("en", "Invalid item")):
            with self.subTest(language=language, name=name):
                baseitems = self.bundled("BaseitemDefinitions")
                target = next(baseitem for baseitem in baseitems["baseitems"] if baseitem["id"] == 1)
                target["name"][language] = name
                result = self.validate_in_edit_dir({"001.jsonc": self.quest()}, "001.jsonc", {"BaseitemDefinitions.jsonc": baseitems})
                self.assert_rejected(result, "unknown or invalid baseitem ID", ["legend", "#", "object"])

    def test_definitions_next_to_the_quest_directory_take_precedence(self):
        monsters = self.bundled("MonraceDefinitions")
        monsters["monsters"] = [monster for monster in monsters["monsters"] if monster["id"] != 1]
        result = self.validate_in_edit_dir({"001.jsonc": self.quest()}, "001.jsonc", {"MonraceDefinitions.jsonc": monsters})
        self.assert_rejected(result, "unknown monster ID", ["definition", "monster"])

    def test_quest_preferences_legend_references_are_checked(self):
        legend = self.quest()["legend"]
        files = {"QuestPreferences.jsonc": {"version": 1, "legend": legend}}
        ok, message = self.validate_in_edit_dir({}, "QuestPreferences.jsonc", files, schema_name="QuestPreferences")
        self.assertTrue(ok, message)
        legend["#"]["artifact"] = 32767
        result = self.validate_in_edit_dir({}, "QuestPreferences.jsonc", files, schema_name="QuestPreferences")
        self.assert_rejected(result, "unknown artifact ID", ["legend", "#", "artifact"])


class QuestMapSymbolTest(QuestEditDirTestCase):
    """CI が QuestList と同じく、どちらの凡例にも無い地図の記号を拒否することを確かめる。"""

    reason = "map symbol is defined neither in the quest legend nor in the shared legend"

    def test_symbols_must_be_in_the_quest_or_shared_legend(self):
        # "." は同梱の共通凡例にあり、"?" はどちらにも無い。
        ok, message = self.validate_in_edit_dir({"001.jsonc": self.quest(map=["#.r"])}, "001.jsonc")
        self.assertTrue(ok, message)
        result = self.validate_in_edit_dir({"001.jsonc": self.quest(map=["#.?"])}, "001.jsonc")
        self.assert_rejected(result, self.reason, ["map", 0, 2])

    def test_unused_variants_are_checked_but_ignored_variants_are_not(self):
        # どの候補が選ばれるかはキャラクタごとに決まるため、使われる候補はすべて確かめる。
        document = self.quest(mapVariants=[["#"], ["#?"]])
        del document["map"]
        self.assert_rejected(self.validate_in_edit_dir({"001.jsonc": document}, "001.jsonc"), self.reason, ["mapVariants", 1, 0, 1])
        # map があれば mapVariants は読まれない。
        ok, message = self.validate_in_edit_dir({"001.jsonc": self.quest(mapVariants=[["?"], ["?"]])}, "001.jsonc")
        self.assertTrue(ok, message)

    def test_shared_legend_next_to_the_quest_directory_takes_precedence(self):
        preferences = {"version": 1, "legend": {"?": {}}}
        files = {"QuestPreferences.jsonc": preferences}
        ok, message = self.validate_in_edit_dir({"001.jsonc": self.quest(map=["#?"])}, "001.jsonc", files)
        self.assertTrue(ok, message)
        result = self.validate_in_edit_dir({"001.jsonc": self.quest(map=["#."])}, "001.jsonc", files)
        self.assert_rejected(result, self.reason, ["map", 0, 1])

    def test_shared_legend_must_keep_symbols_used_by_quests(self):
        quests = {"001.jsonc": self.quest(map=["#."]), "002.jsonc": self.quest(2)}
        files = {"QuestPreferences.jsonc": {"version": 1, "legend": {".": {}}}}
        ok, message = self.validate_in_edit_dir(quests, "QuestPreferences.jsonc", files, schema_name="QuestPreferences")
        self.assertTrue(ok, message)
        files = {"QuestPreferences.jsonc": {"version": 1, "legend": {"X": {}}}}
        ok, message = self.validate_in_edit_dir(quests, "QuestPreferences.jsonc", files, schema_name="QuestPreferences")
        self.assertFalse(ok, message)
        self.assertIn("map symbol '.' at ['map', 0, 1] in 001.jsonc is defined neither in its legend nor in the shared legend", message)
        self.assertIn("Location: ['legend']", message)

    def test_broken_sibling_quests_do_not_stop_other_files(self):
        # 読めない・形の壊れたクエストはそのファイル自身の検証で拒否され、ほかのファイルの検証を例外で止めない。
        broken = {"000.jsonc": [], "003.jsonc": {"id": 3, "mapVariants": 5}}
        files = {"QuestPreferences.jsonc": {"version": 1, "legend": {".": {}}}}
        ok, message = self.validate_in_edit_dir(broken, "QuestPreferences.jsonc", files, schema_name="QuestPreferences",
                                                raw_quests={"004.jsonc": "{\"id\": 4,"})
        self.assertTrue(ok, message)
        quests = broken | {"001.jsonc": self.quest(startVariants=[{"leavingQuest": 3, "y": 0, "x": 0}])}
        ok, message = self.validate_in_edit_dir(quests, "001.jsonc", raw_quests={"004.jsonc": "{\"id\": 4,"})
        self.assertTrue(ok, message)


class LegendEgoSemanticsTest(unittest.TestCase):
    def test_ego_structure_and_attributes(self):
        from jsonschema.exceptions import ValidationError
        from validate_json import validate_legend_ego, extract_baseitem_attributes, extract_ego_attributes
        baseitems = extract_baseitem_attributes({
            "baseitems": [
                {"id": 42, "name": {"ja": "Sword", "en": "Sword"}, "itemkind": {"type_value": 23}},
                {"id": 56, "name": {"ja": "Gold", "en": "Gold"}, "itemkind": {"type_value": 127}},
            ]})
        egos = extract_ego_attributes({"egos": [
            {"id": 65, "rating": 10},
            {"id": 102, "rating": 0},
            {"id": 4, "rating": 30, "flags": ["CURSED"]},
            {"id": 5, "rating": 30, "flags": ["HEAVY_CURSE"]},
            {"id": 6, "rating": 30, "flags": ["PERMA_CURSE"]},
        ]})
        accepted = ({}, {"object": 42, "ego": 65}, {"object": 42, "ego": {"random": True}, "trap": "NONE"})
        rejected = (
            {"object": 42, "ego": 1}, {"object": 42, "ego": 255},
            *({"object": 42, "ego": ego} for ego in (102, 4, 5, 6)),
            {"ego": 65}, {"object": {"random": True}, "ego": 65},
            {"object": {"questReward": True}, "ego": 65},
            {"object": 56, "ego": 65}, {"object": 42, "ego": 65, "trap": "TRAP_PIT"},
            {"object": 42, "ego": {"random": True}, "trap": {"random": True}},
        )
        for cell in accepted:
            validate_legend_ego(cell, baseitems, egos, ["legend", "#"])
        for cell in rejected:
            with self.subTest(cell=cell), self.assertRaises(ValidationError):
                validate_legend_ego(cell, baseitems, egos, ["legend", "#"])


if __name__ == "__main__":
    unittest.main()
