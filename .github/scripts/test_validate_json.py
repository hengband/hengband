"""Regression tests for the actual JSONC validation entry point used in CI."""
import copy
import json
from pathlib import Path
import tempfile
import unittest

from validate_json import load_jsonc, validate_one


class VaultValidationTest(unittest.TestCase):
    def setUp(self):
        self.schema_path = Path(__file__).resolve().parents[2] / "schema/VaultDefinitions.schema.json"
        self.schema = load_jsonc(self.schema_path)
        self.record = {
            "id": 0, "name": "Test", "type": 7, "rating": 5,
            "height": 1, "width": 3, "layout": [" % "],
        }

    def validate(self, data):
        """Use a real temporary JSON file, including schema and semantic checks."""
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "VaultDefinitions.jsonc"
            target.write_text(json.dumps(data), encoding="utf-8")
            return validate_one((target, self.schema_path, self.schema))

    def test_valid_sparse_ids_and_spaces(self):
        second = {**self.record, "id": 3}
        ok, message = self.validate({"vaults": [self.record, second]})
        self.assertTrue(ok, message)

    def test_invalid_roots(self):
        for root in (None, [], {}, {"vaults": None}, {"vaults": {}}, {"vaults": []}):
            with self.subTest(root=root):
                self.assertFalse(self.validate(root)[0])

    def test_duplicate_and_descending_ids(self):
        for ids in ((0, 0), (2, 1)):
            with self.subTest(ids=ids):
                records = [{**self.record, "id": index} for index in ids]
                ok, message = self.validate({"vaults": records})
                self.assertFalse(ok)
                self.assertIn("increasing order", message)
                self.assertIn("['vaults', 1, 'id']", message)

    def test_layout_dimensions(self):
        for layout, reason in (([" % ", " % "], "row count"), (["%"], "row byte length"), (["%%%%"], "row byte length")):
            with self.subTest(layout=layout):
                record = {**self.record, "layout": layout}
                ok, message = self.validate({"vaults": [record]})
                self.assertFalse(ok)
                self.assertIn(reason, message)
                self.assertIn("['vaults', 0, 'layout'", message)

    def test_non_ascii_and_control_characters(self):
        for row in ("%\n", "\u00e9% ", "%\t ", "%\x00 "):
            with self.subTest(row=row):
                record = {**self.record, "layout": [row]}
                self.assertFalse(self.validate({"vaults": [record]})[0])

    def test_integer_representation_matches_reader(self):
        for field in ("id", "type", "rating", "height", "width"):
            with self.subTest(field=field):
                record = copy.deepcopy(self.record)
                record[field] = float(record[field])
                ok, message = self.validate({"vaults": [record]})
                self.assertFalse(ok)
                self.assertIn("integer JSON value", message)

    def test_other_schemas_do_not_require_vault_fields(self):
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder) / "Other.jsonc"
            target.write_text('{"items": []}', encoding="utf-8")
            schema = {"type": "object", "properties": {"items": {"type": "array"}}}
            ok, message = validate_one((target, Path("Other.schema.json"), schema))
            self.assertTrue(ok, message)


if __name__ == "__main__":
    unittest.main()
