#!/usr/bin/env python3
"""
Visual Studio のプロジェクトファイルの整合性をチェックする。

HengbandCore / HengbandTest の vcxproj と filters について:
  - vcxproj の <ClCompile> / <ClInclude> が、それぞれ 1 つの ItemGroup に
    パスの昇順 (大文字小文字を区別しない) で並んでいる
  - filters の内容が、vcxproj の登録内容とファイルのディレクトリから作ったものと一致している
  - CRLF・BOM なしで、ItemGroup の中に空行や行末の空白がない
HengbandCore / HengbandTest / Hengband の vcxproj について:
  - 登録されたファイルが実在し (パスの大文字小文字も一致)、重複がなく、拡張子と種類が合っている
  - 登録内容が src/Makefile.am と一致している (Windows 版で登録しないファイルを除く)
  - テストのソースは HengbandTest に、それ以外は HengbandCore か Hengband のどちらかにある

--fix を付けると、HengbandCore / HengbandTest の vcxproj の並び順や空白を直し、filters を作り直す。
filters は表示用のファイルなので、既存のフィルタの GUID だけを引き継ぎ、ほかの内容
(競合マーカーや余分な子要素など) は捨てる。Hengband.vcxproj は書き換えない。
ファイルの登録漏れや重複など、どう直すべきか決められないものは報告だけ行う。
"""

from __future__ import annotations

import argparse
import collections
import difflib
import functools
import html
import io
import os
import re
import sys
import urllib.parse
import uuid
from pathlib import Path
from xml.etree import ElementTree

REPO_ROOT = Path(__file__).parent.parent.parent
SRC = REPO_ROOT / "src"
VS_DIR = REPO_ROOT / "VisualStudio" / "Hengband"
MAKEFILE_AM = SRC / "Makefile.am"

BOM = b"\xef\xbb\xbf"
NEWLINE = "\r\n"
SRC_PREFIX = "..\\..\\src\\"
SOURCE_EXTENSIONS = {"ClCompile": (".cpp", ".cc"), "ClInclude": (".h", ".hpp")}
SOURCE_TAGS = tuple(SOURCE_EXTENSIONS)
SOURCE_SUFFIXES = sum(SOURCE_EXTENSIONS.values(), ())
# vcxproj の要素のうち、ファイルではないので filters に書かないもの
NON_FILE_TAGS = (
    "ProjectConfiguration",
    "ProjectReference",
    "Reference",
    "PackageReference",
)

ITEM_GROUP_BEGIN = "  <ItemGroup>"
ITEM_GROUP_END = "  </ItemGroup>"
ITEM_PATTERN = re.compile(r'^    <(\w+) Include="([^"]+)"( />|>)$')
SOURCE_ITEM_PATTERN = re.compile(rf"<({'|'.join(SOURCE_TAGS)})\s+Include=")
COMMENT_PATTERN = re.compile(r"<!--.*?-->", re.DOTALL)
FILTER_DEFINITION_PATTERN = re.compile(
    r'<Filter Include="([^"]+)">(?:(?!</Filter>).)*?'
    r"<UniqueIdentifier>(\{[0-9A-Fa-f-]{36}\})</UniqueIdentifier>",
    re.DOTALL,
)

FILTERS_HEADER = [
    '<?xml version="1.0" encoding="utf-8"?>',
    '<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">',
]
FILTERS_FOOTER = ["</Project>", ""]

# --fix の結果との違いを表示する行数の上限
MAX_DIFF_LINES = 20

# テストのソースを置くディレクトリ (src/ からの相対)
TEST_DIR = "test/"

# 整列と filters の整理の対象とするプロジェクトと、フィルタを決める基準のディレクトリ (src/ からの相対)
PROJECTS = {
    "HengbandCore": "",
    "HengbandTest": TEST_DIR,
}

# src/Makefile.am にあっても Windows 版では登録しないファイル (Unix 専用のものと、ヘッダオンリーの外部ライブラリ。
# src/ からの相対パスの前方一致)
WINDOWS_EXCLUDED_SOURCES = (
    "main.cpp",
    "main-gcu.cpp",
    "main-x11.cpp",
    "main-unix/",
    "io/exit-panic.",
    "external-lib/include/",  # ヘッダオンリーの外部ライブラリは登録しない
)

# HengbandTest が HengbandCore と共有し、自分でもコンパイルするファイル (プリコンパイル済みヘッダ)
TEST_SHARED_SOURCES = ("stdafx.cpp", "stdafx.h")


class ParseError(Exception):
    """プロジェクトファイルを読み取れない (自動では直せない) 書式の誤り"""


class Reporter:
    def __init__(self):
        self.errors = 0

    def error(self, path: Path, message: str):
        print(f"{path.relative_to(REPO_ROOT).as_posix()}: error: {message}")
        self.errors += 1


class Item:
    """ItemGroup の要素 1 つ分。直前の XML コメントも raw に含める"""

    def __init__(self, tag: str, include: str, raw: list[str]):
        self.tag = tag
        self.include = include
        self.raw = raw


def item_group_lines(groups: list[list[str]]) -> list[str]:
    """要素の行のリストを ItemGroup で囲んで並べる (空のグループは書かない)"""
    return [
        line
        for group in groups
        if group
        for line in [ITEM_GROUP_BEGIN, *group, ITEM_GROUP_END]
    ]


class ProjectFile:
    """vcxproj / filters のファイル。
    strict でなければ (作り直す filters)、無くても UTF-8 として壊れていても読み進める"""

    def __init__(self, path: Path, strict: bool = False):
        self.path = path
        if not path.exists() and strict:
            raise ParseError("file not found")
        self.original = path.read_bytes() if path.exists() else b""
        try:
            text = self.original.removeprefix(BOM).decode(
                "utf-8", errors="strict" if strict else "replace"
            )
        except UnicodeDecodeError as e:
            raise ParseError(f"not valid UTF-8: {e}") from e
        # 単独の CR も改行とみなし、--fix で CRLF に直す
        self.lines = text.replace(NEWLINE, "\n").replace("\r", "\n").split("\n")


class VcxprojFile(ProjectFile):
    """vcxproj のファイル。ラベルの無い ItemGroup の要素を読み取る"""

    def __init__(self, path: Path):
        super().__init__(path, strict=True)
        # ItemGroup の中の行末の空白は --fix で取り除く
        self.stripped = [line.rstrip() for line in self.lines]
        self.groups = self._parse_groups()

    def _parse_groups(self) -> list[tuple[int, int, list[Item]]]:
        groups = []
        start = None
        for i, line in enumerate(self.stripped):
            if line == ITEM_GROUP_BEGIN:
                if start is not None:
                    raise ParseError(f"line {start + 1}: <ItemGroup> is not closed")
                start = i
            elif line == ITEM_GROUP_END and start is not None:
                groups.append((start, i, self._parse_items(start + 1, i)))
                start = None
        if not groups:
            raise ParseError("no ItemGroup without a label was found")
        # --fix は ItemGroup をまとめて置き換えるので、間に別の要素があると扱えない
        if any(nxt[0] != prev[1] + 1 for prev, nxt in zip(groups, groups[1:])):
            raise ParseError("ItemGroups must be adjacent to each other")
        # 条件付きの ItemGroup などにあるソースは、チェックから漏れないよう誤りとする
        for begin, end in [(0, groups[0][0]), (groups[-1][1] + 1, len(self.lines))]:
            # コメントを (行の数を変えずに) 取り除いてから探す
            text = COMMENT_PATTERN.sub(
                lambda m: "\n" * m.group().count("\n"),
                "\n".join(self.lines[begin:end]),
            )
            for offset, line in enumerate(text.split("\n")):
                if SOURCE_ITEM_PATTERN.search(line):
                    raise ParseError(
                        f"line {begin + offset + 1}: source items must be in an"
                        " ItemGroup without attributes"
                    )
        return groups

    def _parse_items(self, begin: int, end: int) -> list[Item]:
        items = []
        comment = []
        i = begin
        while i < end:
            line = self.stripped[i]
            if not line:
                i += 1  # 空行は --fix で取り除く
                continue
            if line.lstrip().startswith("<!--"):
                comment_start = i
                while "-->" not in self.stripped[i]:
                    comment.append(self.stripped[i])
                    i += 1
                    if i >= end:
                        raise ParseError(
                            f"line {comment_start + 1}: comment is not closed"
                        )
                # コメントの後ろに続く要素は、コメントに紛れて読み飛ばさないよう誤りとする
                if not self.stripped[i].endswith("-->"):
                    raise ParseError(
                        f"line {i + 1}: put an item on its own line after a comment"
                    )
                comment.append(self.stripped[i])
                i += 1
                continue
            m = ITEM_PATTERN.match(line)
            if not m:
                raise ParseError(f"line {i + 1}: unexpected format: {line.strip()}")
            tag, include, close = m.groups()
            raw = comment + [line]
            comment = []
            if close == ">":
                closing = f"    </{tag}>"
                item_start = i
                while self.stripped[i] != closing:
                    i += 1
                    # 次の要素が始まっていれば、閉じ忘れとみなす
                    if i >= end or ITEM_PATTERN.match(self.stripped[i]):
                        raise ParseError(
                            f"line {item_start + 1}: <{tag}> is not closed"
                        )
                    raw.append(self.stripped[i])
            items.append(Item(tag, include, raw))
            i += 1
        # コメントは直後の要素と一緒に並べ替えるため、要素の後に続かないコメントは扱えない
        if comment:
            raise ParseError(
                f"line {end}: a comment at the end of an ItemGroup is not supported;"
                " move it before an item"
            )
        return items

    def items(self, *tags: str) -> list[Item]:
        """tags の要素 (省略時はすべての要素) を返す"""
        return [
            item
            for _, _, items in self.groups
            for item in items
            if not tags or item.tag in tags
        ]

    @property
    def sources(self) -> list[Item]:
        return self.items(*SOURCE_TAGS)

    def replace_groups(self, new_groups: list[list[str]]) -> bytes:
        """ラベルの無い ItemGroup を new_groups で置き換えた内容を、CRLF・BOM なしで返す"""
        first = self.groups[0][0]
        last = self.groups[-1][1]
        lines = (
            self.lines[:first] + item_group_lines(new_groups) + self.lines[last + 1 :]
        )
        return NEWLINE.join(lines).encode("utf-8")


class ListedProject:
    """--fix では書き換えず、登録されたソースを照合するだけのプロジェクト (Hengband.vcxproj)。
    書式は問わないので XML として読む"""

    def __init__(self, path: Path):
        self.path = path
        try:
            root = ElementTree.parse(path).getroot()
        except (OSError, ElementTree.ParseError) as e:
            raise ParseError(str(e)) from e
        self.sources = []
        for element in root.iter():
            tag = element.tag.rpartition("}")[2]
            include = element.get("Include")
            if tag in SOURCE_TAGS and include:
                self.sources.append(Item(tag, include, []))


def load(path: Path, reporter: Reporter, cls: type = VcxprojFile):
    """プロジェクトを読む。読み取れなければ報告して None を返す"""
    try:
        return cls(path)
    except ParseError as e:
        reporter.error(path, str(e))
        return None


def path_key(path: str) -> str:
    """大文字小文字を区別しない並び順のキー。MSBuild の OrdinalIgnoreCase と同じく大文字に寄せる"""
    return path.upper()


def relative_path(include: str) -> str:
    """vcxproj のパスを src/ からの相対パス (区切りは /) にする。
    XML の文字参照と MSBuild のエスケープ (%28 など) は元の文字に戻す"""
    path = urllib.parse.unquote(html.unescape(include))
    return path.removeprefix(SRC_PREFIX).replace("\\", "/")


@functools.cache
def directory_entries(directory: Path) -> tuple[frozenset[str], frozenset[str]]:
    """ディレクトリの中の (サブディレクトリ名, ファイル名) を返す"""
    if not directory.is_dir():
        return frozenset(), frozenset()
    with os.scandir(directory) as entries:
        entries = list(entries)
    dirs = frozenset(e.name for e in entries if e.is_dir())
    files = frozenset(e.name for e in entries if e.is_file())
    return dirs, files


def exists_with_exact_case(rel: str) -> bool:
    *dirs, name = rel.split("/")
    current = SRC
    for part in dirs:
        if part not in directory_entries(current)[0]:
            return False
        current = current / part
    return name in directory_entries(current)[1]


def expected_filter(include: str, filter_root: str) -> str | None:
    """ファイルのディレクトリから、filters に書くべきフィルタを求める。
    基準のディレクトリ (src/ や src/test/) の外にあるファイルはフィルタに入れない"""
    if not include.startswith(SRC_PREFIX):
        return None
    # filters にそのまま書くので、エスケープされたままのパスから求める
    rel = include.removeprefix(SRC_PREFIX).replace("\\", "/")
    if not rel.startswith(filter_root):
        return None
    directory = rel.removeprefix(filter_root).rpartition("/")[0]
    return directory.replace("/", "\\") if directory else None


def group_items(vcxproj: VcxprojFile) -> dict[str, list[Item]]:
    """要素を種類ごとにまとめる。<ClCompile>、<ClInclude> をこの順に昇順に並べ、
    ほかの種類の要素は現れた順に続ける"""
    groups = {
        tag: sorted(vcxproj.items(tag), key=lambda item: path_key(item.include))
        for tag in SOURCE_TAGS
    }
    for item in vcxproj.items():
        if item.tag not in SOURCE_TAGS:
            groups.setdefault(item.tag, []).append(item)
    return groups


def check_vcxproj(
    vcxproj: VcxprojFile, items_by_tag: dict[str, list[Item]], reporter: Reporter
) -> bytes | None:
    """vcxproj の、--fix では直せない誤りをチェックし、整列した内容を返す"""
    sources = [item for tag in SOURCE_TAGS for item in items_by_tag[tag]]
    if not sources:
        reporter.error(
            vcxproj.path,
            "no <ClCompile> / <ClInclude> item was found;"
            " update this script if the format has changed",
        )
        return None
    check_sources(vcxproj.path, sources, reporter)

    # 種類ごとにそれぞれ 1 つの ItemGroup にまとめる
    new_groups = [
        [line for item in items for line in item.raw] for items in items_by_tag.values()
    ]
    return vcxproj.replace_groups(new_groups)


def check_sources(path: Path, sources: list[Item], reporter: Reporter):
    """<ClCompile> / <ClInclude> の重複、パス、ファイルの実在、拡張子をチェックする"""
    registered = collections.defaultdict(list)
    for item in sources:
        registered[path_key(item.include)].append(item.include)
    for includes in sorted(i for i in registered.values() if len(i) > 1):
        reporter.error(
            path, f"{' / '.join(includes)} is registered {len(includes)} times"
        )

    for item in sources:
        if not item.include.startswith(SRC_PREFIX):
            reporter.error(path, f"{item.include}: path should start with {SRC_PREFIX}")
            continue
        if not exists_with_exact_case(relative_path(item.include)):
            reporter.error(
                path,
                f"{item.include}: file does not exist (or the case does not match)",
            )
        if not item.include.endswith(SOURCE_EXTENSIONS[item.tag]):
            reporter.error(
                path, f"{item.include}: should not be registered as <{item.tag}>"
            )


def filtered_item_lines(tag: str, include: str, directory: str | None) -> list[str]:
    """filters に書く、ファイルの要素 (<ClCompile> など) の行"""
    if directory is None:
        return [f'    <{tag} Include="{include}" />']
    return [
        f'    <{tag} Include="{include}">',
        f"      <Filter>{directory}</Filter>",
        f"    </{tag}>",
    ]


def filter_definition_lines(name: str, guid: str) -> list[str]:
    """filters に書く <Filter> の定義の行"""
    return [
        f'    <Filter Include="{name}">',
        f"      <UniqueIdentifier>{guid}</UniqueIdentifier>",
        "    </Filter>",
    ]


def build_filters(
    filters: ProjectFile, items_by_tag: dict[str, list[Item]], filter_root: str
) -> bytes:
    """filters を vcxproj の登録内容 (items_by_tag) から作り直した内容を返す"""
    # 大文字小文字だけが違うディレクトリは、最初に現れた綴りの 1 つのフィルタにまとめる
    spellings = {}

    def unify_case(directory: str) -> str:
        unified = ""
        for part in directory.split("\\"):
            name = f"{unified}\\{part}" if unified else part
            unified = spellings.setdefault(path_key(name), name)
        return unified

    item_groups = []
    needed = set()
    for tag, items in items_by_tag.items():
        if tag in NON_FILE_TAGS:
            continue
        lines = []
        for item in items:
            directory = expected_filter(item.include, filter_root)
            if directory:
                directory = unify_case(directory)
            lines += filtered_item_lines(tag, item.include, directory)
            while directory:
                needed.add(directory)
                directory = directory.rpartition("\\")[0]
        item_groups.append(lines)

    # 既存のフィルタの GUID は、名前の大文字小文字が違っても引き継ぐ (重複していれば作り直す)。
    # 競合マーカーなどで XML として壊れていても、読み取れる定義からは引き継ぐ
    needed_keys = {path_key(name) for name in needed}
    guids = {}
    seen = set()
    for name, guid in FILTER_DEFINITION_PATTERN.findall("\n".join(filters.lines)):
        if path_key(name) not in needed_keys:
            continue  # 消えるフィルタに GUID を取られないようにする
        if path_key(guid) not in seen:
            seen.add(path_key(guid))
            guids.setdefault(path_key(name), guid)
    definitions = []
    for name in sorted(needed, key=path_key):
        # 新しいフィルタの GUID は、誰が --fix を実行しても同じになるよう名前から作る
        new_guid = uuid.uuid5(
            uuid.NAMESPACE_URL, f"hengband:{filters.path.name}:{name}"
        )
        guid = guids.get(path_key(name)) or "{" + str(new_guid) + "}"
        definitions += filter_definition_lines(name, guid)
    lines = FILTERS_HEADER + item_group_lines([definitions, *item_groups])
    return NEWLINE.join(lines + FILTERS_FOOTER).encode("utf-8")


def report_difference(project: ProjectFile, content: bytes, reporter: Reporter):
    """ファイルと --fix の結果との違いを報告する"""
    reasons = []
    if project.original.startswith(BOM):
        reasons.append("has a BOM")
    if project.original.count(b"\n") != project.original.count(b"\r\n") or (
        project.original.count(b"\r") != project.original.count(b"\r\n")
    ):
        reasons.append("newline must be CRLF")
    expected = content.decode("utf-8").split(NEWLINE)
    if project.lines != expected:
        reasons.append("the order, the ItemGroups, the filters or the spaces differ")
    reporter.error(
        project.path, f"differs from what --fix produces: {', '.join(reasons)}"
    )
    diff = difflib.unified_diff(project.lines, expected, n=0, lineterm="")
    diff = list(diff)[2:]
    for line in diff[:MAX_DIFF_LINES]:
        print(f"    {line}")
    if len(diff) > MAX_DIFF_LINES:
        print(f"    ... ({len(diff) - MAX_DIFF_LINES} more lines)")


def read_makefile_am() -> dict[str, list[str]]:
    """src/Makefile.am の変数への代入を、行継続・コメント・追加 (+=) を処理して読み取る"""
    text = MAKEFILE_AM.read_text(encoding="utf-8")
    # automake は ## で始まる行を丸ごと取り除くので、行継続の途中にあっても続きを読む
    text = re.sub(r"^##.*\n", "", text, flags=re.MULTILINE)
    # make と同じく、行継続をつないでから # 以降をコメントとして取り除く
    # (行末が \ のコメントは次の行もコメントになる)
    text = re.sub(r"#.*", "", text.replace("\\\n", " "))
    variables = collections.defaultdict(list)
    depth = 0  # if ... endif の入れ子の深さ
    for line in text.split("\n"):
        if line.startswith("\t"):
            continue  # ルールのレシピ (シェルの if など) は読まない
        directive = line.split(maxsplit=1)[:1]
        if directive == ["if"]:
            depth += 1
        elif directive == ["endif"]:
            depth -= 1
        m = re.match(r"^(\w+)\s*(\+=|:=|\?=|=)(.*)$", line)
        if not m:
            continue
        name, op, value = m.groups()
        if op == "?=" and name in variables:
            continue
        # 条件の中の代入は、どの分岐のファイルも含めるよう追加として扱う
        if op != "+=" and depth == 0:
            variables[name] = []
        variables[name] += value.split()
    return variables


def source_paths(vcxproj: VcxprojFile | ListedProject) -> set[str]:
    return {
        relative_path(item.include)
        for item in vcxproj.sources
        if item.include.startswith(SRC_PREFIX)
    }


def check_makefile_am(
    core: VcxprojFile, test: VcxprojFile, app: ListedProject, reporter: Reporter
):
    variables = read_makefile_am()
    names = (
        "libhengband_a_SOURCES",
        "EXTRA_libhengband_a_SOURCES",
        "hengband_SOURCES",
        "hengband_test_SOURCES",
    )
    for name in names:
        if name not in variables:
            problem = "is not defined; update this script if Makefile.am has been restructured"
        elif any("$(" in path for path in variables[name]):
            problem = "refers to other variables; update this script to expand them"
        else:
            continue
        reporter.error(MAKEFILE_AM, f"{name} {problem}")
        return

    def sources_of(*names: str) -> set[str]:
        return {
            path
            for name in names
            for path in variables[name]
            if path.endswith(SOURCE_SUFFIXES)
        }

    makefile_main = sources_of(
        "libhengband_a_SOURCES", "EXTRA_libhengband_a_SOURCES", "hengband_SOURCES"
    )
    core_paths, app_paths, test_paths = map(source_paths, (core, app, test))
    for path in sorted(core_paths & app_paths):
        reporter.error(app.path, f"{path} is registered in {core.path.name} as well")
    for project, paths in ((core, core_paths), (app, app_paths)):
        for path in sorted(paths):
            if path.startswith(WINDOWS_EXCLUDED_SOURCES):
                reporter.error(
                    project.path,
                    f"{path} is not built on Windows (Unix-only or header-only);"
                    " do not register it",
                )
            elif path.startswith(TEST_DIR):
                reporter.error(
                    project.path,
                    f"{path} is a test source; register it in {test.path.name} instead",
                )
    windows_main = {
        path for path in core_paths | app_paths if not path.startswith(TEST_DIR)
    }
    for path in sorted(windows_main - makefile_main):
        reporter.error(
            MAKEFILE_AM,
            f"{path} is registered in the Visual Studio projects but not in"
            " libhengband_a_SOURCES, EXTRA_libhengband_a_SOURCES (Windows only)"
            " or hengband_SOURCES",
        )
    for path in sorted(makefile_main - windows_main):
        if not path.startswith(WINDOWS_EXCLUDED_SOURCES):
            reporter.error(
                core.path,
                f"{path} is in {MAKEFILE_AM.name} but registered in neither"
                f" {core.path.name} nor {app.path.name}",
            )

    # テストのソースは src/test/ 以下に置き、hengband_test_SOURCES と一致させる。
    # テストのプロジェクトにそれ以外のソースがあれば (TEST_SHARED_SOURCES を除き)、
    # HengbandCore.lib と二重にコンパイルされるので誤りとする
    windows_test = {path for path in test_paths if path.startswith(TEST_DIR)}
    for path in sorted(test_paths - windows_test - set(TEST_SHARED_SOURCES)):
        reporter.error(
            test.path,
            f"{path} is not a test source; register it in {core.path.name} instead",
        )
    makefile_test = sources_of("hengband_test_SOURCES")
    for path in sorted(p for p in makefile_test if not p.startswith(TEST_DIR)):
        reporter.error(
            MAKEFILE_AM,
            f"{path} in hengband_test_SOURCES should be under src/{TEST_DIR}",
        )
    makefile_test = {path for path in makefile_test if path.startswith(TEST_DIR)}
    for path in sorted(windows_test - makefile_test):
        reporter.error(
            MAKEFILE_AM,
            f"{path} is registered in {test.path.name} but not in hengband_test_SOURCES",
        )
    for path in sorted(makefile_test - windows_test):
        reporter.error(
            test.path,
            f"{path} is not registered (it is in hengband_test_SOURCES of {MAKEFILE_AM.name})",
        )


def main() -> int:
    # 差分に日本語のコメントが含まれても、出力の文字コードで表せない文字で落ちないようにする
    if isinstance(sys.stdout, io.TextIOWrapper):
        sys.stdout.reconfigure(errors="backslashreplace")
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--fix", action="store_true", help="sort the items and rebuild the filters"
    )
    args = parser.parse_args()

    reporter = Reporter()
    fixes: list[tuple[ProjectFile, bytes]] = []
    projects = {}
    for name, filter_root in PROJECTS.items():
        vcxproj = load(VS_DIR / f"{name}.vcxproj", reporter)
        if vcxproj is None:
            # vcxproj を読み取れなければ、それから作る filters のチェックも行わない
            continue
        projects[name] = vcxproj
        # vcxproj と filters は、同じ並びの要素から作る
        items_by_tag = group_items(vcxproj)
        content = check_vcxproj(vcxproj, items_by_tag, reporter)
        if content is None:
            # vcxproj を整列できない (ソースが見つからないなど) 場合、filters も作り直さない
            continue
        fixes.append((vcxproj, content))
        filters = ProjectFile(VS_DIR / f"{name}.vcxproj.filters")
        fixes.append((filters, build_filters(filters, items_by_tag, filter_root)))

    # Hengband.vcxproj は Makefile.am との照合にだけ使い、--fix では書き換えない
    app = load(VS_DIR / "Hengband.vcxproj", reporter, ListedProject)
    if app:
        check_sources(app.path, app.sources, reporter)
    core, test = projects.get("HengbandCore"), projects.get("HengbandTest")
    if core and test and app:
        check_makefile_am(core, test, app, reporter)

    # ここまでのエラーは、どれも --fix では直せない
    changed = [(p, c) for p, c in fixes if c != p.original]
    if args.fix:
        for project, content in changed:
            project.path.write_bytes(content)
            print(f"{project.path.relative_to(REPO_ROOT).as_posix()}: fixed")
        if reporter.errors > 0:
            print(f"\n{reporter.errors} error(s) need to be fixed by hand.")
            return 1
        return 0

    for project, content in changed:
        report_difference(project, content, reporter)
    if reporter.errors > 0:
        print(f"\n{reporter.errors} error(s) found.")
        if changed:
            print(
                "Some of them can be fixed by: python3 .github/scripts/check-vcxproj.py --fix"
            )
        return 1

    print("Visual Studio project check: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
