"""JSONCのCI検証入口でモンスター種族の読込規則を確認する。"""
import copy
from pathlib import Path
import unittest

from json_validation_test_helper import validate_document
from validate_json import build_schema_registry, load_all_schemas, load_jsonc


class MonraceValidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[2]
        cls.loaded, schema_map = load_all_schemas(cls.root / "schema")
        cls.schema_path = schema_map["MonraceDefinitions"]
        cls.registry = build_schema_registry(cls.loaded)
        cls.document = load_jsonc(cls.root / "lib/edit/MonraceDefinitions.jsonc")

    def validate(self, monsters):
        document = {"versions": self.document["versions"], "monsters": monsters}
        return validate_document(document, self.schema_path, self.loaded[self.schema_path], self.registry)

    def monster(self, monster_id=1, **fields):
        monster = copy.deepcopy(self.document["monsters"][1])
        for key in ("evolve", "artifacts", "escorts", "final_summon", "message"):
            monster.pop(key, None)
        monster["id"] = monster_id
        monster.update(fields)
        return monster

    def assert_valid(self, monsters):
        ok, diagnostic = self.validate(monsters)
        self.assertTrue(ok, diagnostic)

    def assert_invalid(self, monsters, path, reason=None):
        ok, diagnostic = self.validate(monsters)
        self.assertFalse(ok, diagnostic)
        self.assertIn(f"Location: {path}", diagnostic)
        if reason:
            self.assertIn(reason, diagnostic)

    def test_bundled_monsters(self):
        self.assert_valid(self.document["monsters"])

    def test_ids_must_be_strictly_increasing(self):
        self.assert_valid([self.monster(1), self.monster(3)])
        for second_id in (1, 0):
            with self.subTest(second_id=second_id):
                self.assert_invalid([self.monster(1), self.monster(second_id)], ["monsters", 1, "id"], "strictly increasing")


if __name__ == "__main__":
    unittest.main()
