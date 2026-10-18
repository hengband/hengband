"""荒野・町の一覧の schema と意味検証が、読み込みと実行時の決まりに合うことを確かめる。"""
import copy
from pathlib import Path
import re
import tempfile
import unittest

from jsonschema import ValidationError

from json_validation_test_helper import validate_document
from validate_json import REQUIRED_TOWN_IDS, count_town_files, load_jsonc, validate_wilderness_semantics


def make_wilderness():
    town_name = {"ja": "街", "en": "Town"}
    return {
        "version": 1,
        "width": 3,
        "height": 3,
        "towns": [{"id": 1, "name": town_name, "alias": town_name}] + [{"id": town_id, "name": town_name} for town_id in range(2, 6)],
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


class WildernessTownValidationTest(unittest.TestCase):
    def setUp(self):
        self.schema_path = Path(__file__).resolve().parents[2] / "schema/WildernessDefinition.schema.json"
        self.schema = load_jsonc(self.schema_path)
        self.data = make_wilderness()

    def validate(self, data):
        return validate_document(data, self.schema_path, self.schema, filename="WildernessDefinition.jsonc")

    def test_bundled_town_files_allow_the_largest_town_id(self):
        # 町の訪問記録は TownId::MAX（6）までの町しか持たない。
        data = copy.deepcopy(self.data)
        data["towns"].append({"id": 6, "name": {"ja": "街", "en": "Town"}})
        ok, message = self.validate(data)
        self.assertTrue(ok, message)
        data["towns"][-1]["id"] = 7
        ok, message = self.validate(data)
        self.assertFalse(ok, message)
        self.assertIn("Location: ['towns', 5, 'id']", message)

    def test_schema_town_limits_match_town_records(self):
        # 町の訪問記録（TownRecords）は TownId::MAX までの町しか持たないため、schema の上限をそれに揃える。
        header = (Path(__file__).resolve().parents[2] / "src/system/floor/town-records.h").read_text(encoding="utf-8")
        enum_body = re.search(r"enum class TownId \{(.*?)\};", header, re.DOTALL)[1]
        names = [entry.split("=")[0].strip() for entry in enum_body.split(",") if entry.strip()]
        town_id_max = names.index("MAX")
        self.assertEqual(self.schema["definitions"]["town"]["properties"]["id"]["maximum"], town_id_max)
        self.assertEqual(self.schema["definitions"]["letter"]["properties"]["town"]["maximum"], town_id_max)

    def test_required_towns_match_valid_towns(self):
        # 初期化（TownList::is_all_initialized）は町1〜VALID_TOWNS-1 の名前を求める。
        header = (Path(__file__).resolve().parents[2] / "src/system/floor/town-list.h").read_text(encoding="utf-8")
        valid_towns = int(re.search(r"constexpr size_t VALID_TOWNS = (\d+);", header)[1])
        self.assertEqual(REQUIRED_TOWN_IDS, range(1, valid_towns))

    def test_letter_town_follows_the_town_id_limit(self):
        for map_name in ("normal", "compact"):
            with self.subTest(map_name=map_name):
                data = copy.deepcopy(self.data)
                data["maps"][map_name]["letters"][0]["town"] = 7
                ok, message = self.validate(data)
                self.assertFalse(ok, message)
                self.assertIn(f"Location: ['maps', '{map_name}', 'letters', 0, 'town']", message)

    def test_towns_one_to_five_and_the_alias_of_town_one_are_required(self):
        # TownList::is_all_initialized() は町1〜5の名前と町1の別名を求め、無ければ初期化で終了する。
        for town_id in range(1, 6):
            with self.subTest(town_id=town_id):
                data = copy.deepcopy(self.data)
                data["towns"] = [town for town in data["towns"] if town["id"] != town_id]
                ok, message = self.validate(data)
                self.assertFalse(ok, message)
                self.assertIn(f"town {town_id} must be defined", message)
        data = copy.deepcopy(self.data)
        del data["towns"][0]["alias"]
        ok, message = self.validate(data)
        self.assertFalse(ok, message)
        self.assertIn("town 1 must have an alias", message)

    def test_wilderness_edge_must_be_closed(self):
        # 外周が通れると、荒野の端の区画に入ったときに範囲外を引いて落ちる。
        edge_letters = (
            {"symbol": "x", "terrain": 5},
            {"symbol": "x", "terrain": 0, "town": 1},
            {"symbol": "x", "terrain": 0, "road": 1},
        )
        for map_name in ("normal", "compact"):
            for letter in edge_letters:
                for row_index, row, column in ((1, "x##", 0), (1, "##x", 2), (0, "#x#", 1), (2, "#x#", 1)):
                    with self.subTest(map_name=map_name, letter=letter, row=row):
                        data = copy.deepcopy(self.data)
                        data["maps"][map_name]["letters"].append(letter)
                        data["maps"][map_name]["layout"][row_index] = row
                        ok, message = self.validate(data)
                        self.assertFalse(ok, message)
                        self.assertIn(f"Location: ['maps', '{map_name}', 'layout', {row_index}, {column}]", message)
        data = copy.deepcopy(self.data)
        data["maps"]["compact"]["letters"].append({"symbol": "x", "terrain": 5, "road": 1})
        data["maps"]["compact"]["layout"][1] = "#x#"
        ok, message = self.validate(data)
        self.assertTrue(ok, message)
        # 簡易版の短い行でも、荒野の幅で決まる外周（右の列）にあるマスは確かめる。
        data = copy.deepcopy(self.data)
        data["width"] = 4
        data["maps"]["normal"]["layout"] = ["####", "#1##", "####"]
        data["maps"]["compact"]["letters"].append({"symbol": "x", "terrain": 5})
        data["maps"]["compact"]["layout"] = ["####", "##x", "###"]
        ok, message = self.validate(data)
        self.assertTrue(ok, message)
        data["maps"]["compact"]["layout"] = ["####", "###x", "###"]
        ok, message = self.validate(data)
        self.assertFalse(ok, message)
        self.assertIn("Location: ['maps', 'compact', 'layout', 1, 3]", message)

    def test_letter_level_follows_the_maximum_depth(self):
        # 生成は階層を 127（MAX_DEPTH - 1）で頭打ちにし、地上の階層はセーブで16ビットになる。
        for level, accepted in ((127, True), (128, False), ({"ja": 127, "en": 127}, True), ({"ja": 128, "en": 1}, False), ({"ja": 1, "en": 128}, False)):
            with self.subTest(level=level):
                data = copy.deepcopy(self.data)
                data["maps"]["normal"]["letters"][1]["level"] = level
                ok, message = self.validate(data)
                self.assertEqual(ok, accepted, message)

    def test_town_ids_must_have_town_definition_files(self):
        # 初期化は、町の定義ファイル名の先頭2文字の種類の数を超える番号の町を拒否する。
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            towns = root / "lib/edit/towns"
            towns.mkdir(parents=True)
            for name in ("01_a.jsonc", "01_b.jsonc", "02_c.jsonc", "03_d.jsonc", "04_e.jsonc", "05_f.jsonc", "readme.txt"):
                (towns / name).write_text("{}", encoding="utf-8")
            self.assertEqual(count_town_files(root), 5)
            schema_path = root / "schema/WildernessDefinition.schema.json"
            validate_wilderness_semantics(self.data, schema_path)
            data = copy.deepcopy(self.data)
            data["towns"].append({"id": 6, "name": {"ja": "街", "en": "Town"}})
            with self.assertRaises(ValidationError) as raised:
                validate_wilderness_semantics(data, schema_path)
            self.assertEqual(list(raised.exception.path), ["towns", 5, "id"])
            self.assertIn("town definition file", raised.exception.message)


if __name__ == "__main__":
    unittest.main()
