"""CI入口へ一時JSON文書を渡すテスト用の共通処理。"""
import json
from pathlib import Path
import re
import tempfile

from validate_json import load_cpp_enum_values, load_cpp_map_keys, validate_one

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


def quest_id_schema_maximum():
    """クエストIDを参照する項目のスキーマの上限（QuestId の基になる型の上限−1）を返す。"""
    return cpp_storage_maximum("src/system/dungeon/quest-definition.h", r"enum class QuestId : (\w+) \{") - 1


def dungeon_id_maximum():
    """ダンジョンIDの上限（DungeonId::MAX − 1）を返す。"""
    relative_path = "src/system/enums/dungeon/dungeon-id.h"
    # 列挙子の値の読み方（明示した "= N"、無ければ直前の値+1）は validate_json.py と共通にする。
    values = load_cpp_enum_values(REPOSITORY_ROOT / relative_path, "DungeonId")
    if "MAX" not in values:
        raise AssertionError(f"{relative_path}: DungeonId has no MAX")
    return values["MAX"] - 1


def schema_enum_values(node, schema_path, loaded_schemas):
    """スキーマの節が受け付ける列挙値を、$ref と anyOf をたどってすべて集める。

    共通の定義 (schema/common/) を参照する列挙も、参照先の列挙と各スキーマで足した値を合わせて読む。
    """
    if "$ref" in node:
        target, _, pointer = node["$ref"].partition("#")
        if target:
            schema_path = (schema_path.parent / target).resolve()
            schemas = {path.resolve(): schema for path, schema in loaded_schemas.items()}
            if schema_path not in schemas:
                raise AssertionError(f"unknown schema reference {node['$ref']!r}")
            referenced = schemas[schema_path]
        else:
            referenced = loaded_schemas[schema_path]
        for part in filter(None, pointer.split("/")):
            referenced = referenced[part.replace("~1", "/").replace("~0", "~")]
        return schema_enum_values(referenced, schema_path, loaded_schemas)
    if "enum" in node:
        return set(node["enum"])
    if "anyOf" in node:
        values = set()
        for branch in node["anyOf"]:
            values |= schema_enum_values(branch, schema_path, loaded_schemas)
        return values
    raise AssertionError(f"schema node has no enum: {node!r}")


# 種族とダンジョンの読み込みが共有する、モンスターのフラグ名を照合する関数と、その定義のあるファイル。
SHARED_MONSTER_FLAG_FUNCTIONS = {
    "grab_one_monster_basic_flag": "src/info-reader/monster-flag-reader.cpp",
    "grab_one_monster_spell_flag": "src/info-reader/monster-flag-reader.cpp",
}


def reader_token_tables(relative_path, function):
    """読み込み処理の指定関数が grab_one_flag で照合する辞書名を、照合の順に読む。

    共有の照合関数（SHARED_MONSTER_FLAG_FUNCTIONS）を呼んでいれば、その関数が照合する辞書も呼んだ位置の順に含める。
    """
    source = (REPOSITORY_ROOT / relative_path).read_text(encoding="utf-8")
    match = re.search(r"\b" + re.escape(function) + r"\(.*?\n\{(.*?)^\}", source, re.DOTALL | re.MULTILINE)
    if match is None:
        raise AssertionError(f"{function} not found in {relative_path}")
    shared_calls = "|".join(re.escape(name) for name in SHARED_MONSTER_FLAG_FUNCTIONS)
    pattern = r"grab_one_flag\(\s*\*?[\w.]+\s*,\s*(r_info_\w+)\s*,|\b(" + shared_calls + r")\("
    tables = []
    for table, shared in re.findall(pattern, match[1]):
        if table:
            tables.append(table)
        elif shared != function:
            tables.extend(reader_token_tables(SHARED_MONSTER_FLAG_FUNCTIONS[shared], shared))
    if not tables:
        raise AssertionError(f"no token tables found in {function}")
    return tables


def race_info_table_keys(table):
    """race-info-tokens-table.cpp の指定した辞書のトークンを読む。"""
    return load_cpp_map_keys(REPOSITORY_ROOT / "src/info-reader/race-info-tokens-table.cpp", table)
