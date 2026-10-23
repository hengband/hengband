"""CI入口へ一時JSON文書を渡すテスト用の共通処理。"""
import json
from pathlib import Path
import re
import tempfile

from validate_json import validate_one

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]

# 定義のIDを格納するC++の整数型と、その型の上限。
CPP_INTEGER_MAXIMUMS = {"short": 32767, "int16_t": 32767}


def validate_document(document, schema_path, schema, registry=None, *, filename=None, companions=None):
    with tempfile.TemporaryDirectory() as folder:
        directory = Path(folder)
        target = directory / (filename or schema_path.name.removesuffix(".schema.json") + ".jsonc")
        target.write_text(json.dumps(document), encoding="utf-8")
        for name, data in (companions or {}).items():
            (directory / name).write_text(json.dumps(data), encoding="utf-8")
        return validate_one((target, schema_path, schema), registry)


def cpp_storage_maximum(relative_path, pattern):
    """C++のソースからIDを格納する整数型を読み、その型の上限を返す。

    pattern は型名を1つ目のグループに持つ正規表現。見つからない場合や型が未知の場合は AssertionError にする。
    """
    source = (REPOSITORY_ROOT / relative_path).read_text(encoding="utf-8")
    match = re.search(pattern, source)
    if match is None:
        raise AssertionError(f"{relative_path} has no declaration matching {pattern!r}")
    type_name = match[1]
    if type_name not in CPP_INTEGER_MAXIMUMS:
        raise AssertionError(f"{relative_path}: unknown storage type {type_name!r}")
    return CPP_INTEGER_MAXIMUMS[type_name]


# 定義のIDの上限は、IDを格納する型の上限−1とする。IDの最大値+1になる件数も同じ型に収まるようにするため。
def baseitem_id_schema_maximum():
    """ベースアイテムIDのスキーマの上限（BaseitemList がIDを扱う型の上限−1）を返す。"""
    return cpp_storage_maximum("src/system/baseitem/baseitem-list.h", r"bool is_valid\((\w+) bi_id\) const;") - 1


def monrace_id_schema_maximum():
    """モンスター種族IDのスキーマの上限（MonraceId の基になる型の上限−1）を返す。"""
    return cpp_storage_maximum("src/system/enums/monrace/monrace-id.h", r"enum class MonraceId : (\w+) \{") - 1


def fixed_artifact_id_schema_maximum():
    """固定アーティファクトIDのスキーマの上限（FixedArtifactId の基になる型の上限−1）を返す。"""
    return cpp_storage_maximum("src/artifact/fixed-art-types.h", r"enum class FixedArtifactId : (\w+) \{") - 1


def terrain_id_schema_maximum():
    """地形IDのスキーマの上限（FEAT_IDX の上限−1）を返す。"""
    return cpp_storage_maximum("src/system/h-type.h", r"typedef (\w+) FEAT_IDX;") - 1
