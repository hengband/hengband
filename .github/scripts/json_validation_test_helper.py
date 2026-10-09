"""CI入口へ一時JSON文書を渡すテスト用の共通処理。"""
import json
from pathlib import Path
import tempfile

from validate_json import validate_one


def validate_document(document, schema_path, schema, registry=None, *, filename=None, companions=None):
    with tempfile.TemporaryDirectory() as folder:
        directory = Path(folder)
        target = directory / (filename or schema_path.name.removesuffix(".schema.json") + ".jsonc")
        target.write_text(json.dumps(document), encoding="utf-8")
        for name, data in (companions or {}).items():
            (directory / name).write_text(json.dumps(data), encoding="utf-8")
        return validate_one((target, schema_path, schema), registry)
