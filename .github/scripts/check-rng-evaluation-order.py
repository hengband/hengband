#!/usr/bin/env python3
"""
乱数を引く順序がコンパイラによって変わる式を検出する。

C++ では、関数やコンストラクタの引数の評価順と、多くの二項演算子の両辺の評価順が
規定されていない。そこで乱数を 2 回以上引くと、引く順序がコンパイラによって変わり
(GCC は右の引数から、clang は左の引数から評価する)、同じセーブ・同じキー列でも
ゲームの進行が食い違う (#5726)。

次の式を clang-query で検出する。
- 関数・コンストラクタの 2 つ以上の引数で、乱数を引く関数を呼んでいる
  (波括弧の初期化は左から順に評価されると規定されているので対象外)
- 評価順の規定されていない二項演算子の両辺で、乱数を引く関数を呼んでいる

全ファイルの解析には時間がかかる (4 コアで約 20 分) ため、PR では変更されたファイルだけを
解析し (--base)、全ファイルは定期実行で解析する。対象の式は乱数を引く関数を直接呼んで
いるものに限られるので、新たに入り込むのはそのファイルを変更したときに限られる。
ただし、このスクリプトやワークフローを変更したときは検出の仕方が変わるので、全ファイルを解析する。

全ファイルを解析するときは .cpp だけを解析する (取り込んだヘッダの中も一緒に調べられる)。
変更されたファイルだけを解析するときは、変更されたヘッダをそれを取り込む .cpp で解析する
(ヘッダを単独で解析すると、#pragma once が効かず循環して取り込まれるヘッダが二重に定義されたり、
先に取り込まれるものに頼るヘッダが解析できなかったりする)。
日本語版の定義で解析し、JP の定義によって変わる部分 (JP を見る #if 系の中) に乱数を引く関数の名前が
あるか、その部分で取り込むファイルの先に名前があるファイルを (間接的にでも) 取り込んでいるものは、
英語版の定義でも解析する (英語版でだけ解析される呼び出しは、必ずどちらかに書かれている)。

乱数を引く関数を間接的に呼ぶ関数 (モンスターの配置など) と、乱数を引くマクロ
(ENERGY_NEED) の中身の変更による影響は、変更されたファイルだけの解析では対象にならない
(後者は全ファイルの解析で検出する)。

configure は不要。--extra-arg は、手元の clang が既定で選ぶ標準ライブラリを解析できない
場合などに使う (例: --extra-arg=--gcc-install-dir=/usr/lib/gcc/x86_64-linux-gnu/14)。
"""

import argparse
import collections
import functools
import concurrent.futures
import itertools
import os
import pathlib
import re
import subprocess
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent

# ゲームの乱数生成器を回す関数
# クラステンプレートのメンバー関数は、修飾名にテンプレート引数が入って一致しないので、修飾しない名前で書く
RNG_FUNCTIONS = [
    "::randint0",
    "::randint1",
    "::randnum0",
    "::randnum1",
    "::rand_range",
    "::rand_spread",
    "::randnor",
    "::evaluate_percent",
    "::one_in_",
    "::rand_choice",
    "::rand_shuffle",
    "::div_round",
    "::Dice::roll",
    "::get_random_line",
    "pick_one_at_random",
    "pick_id_at_random",
    "pick_monrace_at_random",
]

# 両辺 (または引数) の評価順が規定されている演算子
SEQUENCED_OPERATORS = ["=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "<<", ">>", "&&", "||", ",", "[]", "->*"]

# 引数の組を調べる引数の数の上限 (hasArgument は番号を指定して照合するので、上限が要る)
# これより引数の多い呼び出しは、どれかの引数で乱数を引いていれば調べきれないものとして検出する
MAX_ARGS = 16

# ゲームの進行に関わらず、ほかの環境のヘッダが要るフロントエンドと、外部のライブラリは除く
EXCLUDED_PREFIXES = ("src/main-", "src/external-lib/")

# configure の代わりに、解析に要るものだけを定義する
COMPILE_ARGS = ["-std=c++20", "-DHAVE_SYS_TIME_H", "-Isrc", "-isystem", "src/external-lib/include"]
JAPANESE_ARGS = ("-DJP", "-DEUC")
ENGLISH_ARGS = ()

# 取り込む .cpp が無いヘッダを単独で解析するときの引数
HEADER_ARGS = ["-x", "c++", "-Wno-pragma-once-outside-header"]

CONDITIONAL_PATTERN = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)")
JP_PATTERN = re.compile(r"\bJP\b")
INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
RNG_NAME_PATTERN = re.compile(r"\b(?:" + "|".join(name.split("::")[-1] for name in RNG_FUNCTIONS) + r"|ENERGY_NEED)\b")

# 変更されると検出の仕方が変わるので、全ファイルを解析するファイル
FULL_CHECK_TRIGGERS = [
    pathlib.Path(__file__).resolve().relative_to(REPO_ROOT),
    pathlib.Path(".github/workflows/check-rng-evaluation-order.yml"),
]

BIND_PATTERN = re.compile(r'^(?P<file>[^:\s]+):(?P<line>\d+):\d+: note: "(?P<kind>call|ctor|binop|many)" binds here$')
ERROR_PATTERN = re.compile(r": (?:fatal )?error: ")


def argument_pairs(first):
    """first 番目以降の引数のうち、2 つの引数で乱数を引いていることを表す matcher"""
    return ", ".join(f"allOf(hasArgument({i}, hasRng), hasArgument({j}, hasRng))" for i, j in itertools.combinations(range(first, MAX_ARGS), 2))


def build_query():
    """clang-query に -c で渡すコマンドの列を返す"""
    names = ", ".join(f'"{name}"' for name in RNG_FUNCTIONS)
    member_names = ", ".join(f'hasMemberName("{name.split("::")[-1]}")' for name in RNG_FUNCTIONS)
    operators = ", ".join(f'"{op}"' for op in SEQUENCED_OPERATORS)
    return [
        "set output diag",
        "set bind-root false",
        "set traversal IgnoreUnlessSpelledInSource",
        # テンプレートの中で引数が依存していると呼び先が決まらないので、候補の宣言やメンバー名でも照合する
        f"let rngDecl namedDecl(hasAnyName({names}))",
        f"let rngCall callExpr(anyOf(callee(rngDecl), callee(unresolvedLookupExpr(hasAnyDeclaration(rngDecl))), callee(cxxDependentScopeMemberExpr(anyOf({member_names})))))",
        # sizeof・alignof・noexcept のオペランドは評価されないので、中の呼び出しは数えない
        "let unevaluated expr(anyOf(unaryExprOrTypeTraitExpr(), cxxNoexceptExpr()))",
        # 検出対象の式 (outer。各 match の先頭で束縛する) に引数として渡すだけのラムダは、引数をすべて評価した
        # 後で呼ばれるので、本体の中の呼び出しは数えない。キャプチャの初期化、その場で呼び出すラムダ、内側の
        # 呼び出しに渡したラムダは、引数の評価中に実行されうるので数える
        'let rngCallEvaluated callExpr(rngCall, unless(hasAncestor(compoundStmt(hasParent(lambdaExpr(hasParent(expr(equalsBoundNode("outer")))))))), unless(hasAncestor(unevaluated)))',
        "let hasRng expr(anyOf(rngCallEvaluated, hasDescendant(rngCallEvaluated)))",
        # 配下に乱数の呼び出しが無い式を先に除いてから、引数の組を調べる
        f'match callExpr(expr().bind("outer"), hasDescendant(rngCall), unless(cxxOperatorCallExpr(hasAnyOverloadedOperatorName({operators}, "()"))), anyOf({argument_pairs(0)})).bind("call")',
        # operator() の 0 番目の引数は呼び出す対象で、ほかの引数より先に評価されるので比べない
        f'match cxxOperatorCallExpr(expr().bind("outer"), hasOverloadedOperatorName("()"), hasDescendant(rngCall), anyOf({argument_pairs(1)})).bind("call")',
        f'match cxxConstructExpr(expr().bind("outer"), hasDescendant(rngCall), unless(isListInitialization()), anyOf({argument_pairs(0)})).bind("ctor")',
        f'match binaryOperator(expr().bind("outer"), unless(hasAnyOperatorName({operators})), hasLHS(hasRng), hasRHS(hasRng)).bind("binop")',
        # C++20 で operator== などから書き換えられた比較 (a != b など)
        'match cxxRewrittenBinaryOperator(expr().bind("outer"), hasLHS(hasRng), hasRHS(hasRng)).bind("binop")',
        f'match callExpr(expr().bind("outer"), hasArgument({MAX_ARGS}, anything()), hasAnyArgument(hasRng)).bind("many")',
        f'match cxxConstructExpr(expr().bind("outer"), hasArgument({MAX_ARGS}, anything()), hasAnyArgument(hasRng)).bind("many")',
    ]


def is_source(path):
    return path.suffix in (".cpp", ".h") and str(path).startswith("src/") and not str(path).startswith(EXCLUDED_PREFIXES)


def list_sources(base):
    """解析するファイルを返す。base を指定すると、base との分岐点から変更されたファイルに限る"""
    if base:
        command = ["git", "diff", "--name-only", "--diff-filter=d", f"{base}...HEAD"]
        result = subprocess.run(command, capture_output=True, text=True, cwd=REPO_ROOT, check=True)
        changed = [pathlib.Path(line) for line in result.stdout.splitlines()]
        triggers = [path for path in FULL_CHECK_TRIGGERS if path in changed]
        if not triggers:
            return sorted(path for path in changed if is_source(path))

        print(f"{triggers[0]} が変更されているので、全ファイルを解析する", file=sys.stderr)

    # 全ファイルを解析するときは .cpp だけを解析する
    sources = (path.relative_to(REPO_ROOT) for path in (REPO_ROOT / "src").rglob("*.cpp"))
    return sorted(path for path in sources if is_source(path))


@functools.cache
def read_source(path):
    return (REPO_ROOT / path).read_text(encoding="utf-8", errors="replace")


class IncludeGraph:
    """ファイルの取り込みの関係と、JP の定義によって変わる部分の情報"""

    def __init__(self):
        self.includes = collections.defaultdict(list)  # 取り込むファイル -> 取り込まれるファイル
        # 取り込まれるファイル -> 取り込むファイル。#if の中の取り込みは含めない
        # (ヘッダを解析する .cpp を探すのに使う。解析に使う定義によってはヘッダが取り込まれず、黙って検出が漏れるため)
        self.includers = collections.defaultdict(list)
        self.jp_dependent_rng = set()  # JP の定義によって変わる部分に乱数を引く関数の名前があるファイル
        self.jp_dependent_includes = collections.defaultdict(list)  # JP の定義によって変わる部分で取り込むファイル
        for source in sorted(p.relative_to(REPO_ROOT) for p in (REPO_ROOT / "src").rglob("*")):
            if is_source(source):
                self.parse(source)

    def parse(self, source):
        is_jp_conditional = []  # 開いている #if 系ごとに、JP を見るかどうか
        for line in read_source(source).splitlines():
            if match := CONDITIONAL_PATTERN.match(line):
                directive, condition = match.groups()
                if directive in ("if", "ifdef", "ifndef"):
                    is_jp_conditional.append(bool(JP_PATTERN.search(condition)))
                elif directive == "elif" and is_jp_conditional and JP_PATTERN.search(condition):
                    is_jp_conditional[-1] = True
                elif directive == "endif" and is_jp_conditional:
                    is_jp_conditional.pop()
                continue

            jp_dependent = any(is_jp_conditional)
            if jp_dependent and RNG_NAME_PATTERN.search(line):
                self.jp_dependent_rng.add(source)

            if match := INCLUDE_PATTERN.match(line):
                included = pathlib.Path("src") / match[1]
                self.includes[source].append(included)
                if not is_jp_conditional:
                    self.includers[included].append(source)
                if jp_dependent:
                    self.jp_dependent_includes[source].append(included)

    def closure(self, path):
        """path と、path が (間接的にでも) 取り込むファイル"""
        visited = {path}
        stack = [path]
        while stack:
            for included in self.includes[stack.pop()]:
                if included not in visited:
                    visited.add(included)
                    stack.append(included)

        return visited

    @functools.cache
    def reaches_rng(self, path):
        """path か、path が取り込むファイルに乱数を引く関数の名前があるか"""
        return any((REPO_ROOT / f).is_file() and RNG_NAME_PATTERN.search(read_source(f)) for f in self.closure(path))

    def needs_english(self, path):
        """英語版の定義でも解析する必要があるか"""
        for f in self.closure(path):
            if f in self.jp_dependent_rng or any(self.reaches_rng(included) for included in self.jp_dependent_includes[f]):
                return True

        return False


def find_includer(header, includers, preferred):
    """header を (間接的に) 取り込む .cpp を 1 つ返す。見つからなければ None を返す"""
    # すでに解析する .cpp (preferred)、ヘッダと同じ名前の .cpp、近いものの順に選ぶ
    found = []
    visited = {header}
    queue = collections.deque([header])
    while queue:
        for path in includers[queue.popleft()]:
            if path in visited:
                continue

            visited.add(path)
            if path.suffix == ".cpp":
                found.append(path)
            else:
                queue.append(path)

    for path in found:
        if path in preferred:
            return path

    same_name = header.with_suffix(".cpp")
    if same_name in found:
        return same_name

    return found[0] if found else None


def list_targets(sources):
    """clang-query にかけるファイルと、解析に使う定義の組を返す"""
    graph = IncludeGraph()
    targets = []
    for path in sources:
        analyzed = path
        if path.suffix == ".h":
            # 取り込む .cpp が無い (まだどこからも取り込まれていない) ヘッダは単独で解析する
            analyzed = find_includer(path, graph.includers, sources) or path

        targets.append((analyzed, JAPANESE_ARGS))
        if graph.needs_english(analyzed):
            targets.append((analyzed, ENGLISH_ARGS))

    # 同じ .cpp を重ねて解析しないよう、重複を除く
    return list(dict.fromkeys(targets))


def run_clang_query(clang_query, query, extra_args, path, define_args):
    file_args = HEADER_ARGS if path.suffix == ".h" else []
    command = [clang_query] + [f"-c={command}" for command in query] + [str(path), "--"] + file_args + COMPILE_ARGS + list(define_args) + extra_args
    result = subprocess.run(command, capture_output=True, text=True, cwd=REPO_ROOT)
    return path, result.returncode, result.stdout + result.stderr


def main():
    parser = argparse.ArgumentParser(description="乱数を引く順序がコンパイラによって変わる式を検出する")
    parser.add_argument("--base", help="この REF との分岐点から変更されたファイルだけを解析する")
    parser.add_argument("--clang-query", default="clang-query", help="clang-query のパス (既定: clang-query)")
    parser.add_argument("--extra-arg", action="append", default=[], help="コンパイラに追加で渡す引数 (複数指定可)")
    parser.add_argument("--list", action="store_true", help="解析するファイルを表示するだけで、解析はしない")
    args = parser.parse_args()

    sources = list_sources(args.base)
    if args.list:
        for path in sources:
            print(path)
        return 0

    targets = list_targets(sources)
    print(f"{len(sources)} ファイルを clang-query で調べる (英語版を含めて {len(targets)} 回)")

    findings = set()
    failed = False
    query = build_query()
    with concurrent.futures.ThreadPoolExecutor() as executor:
        futures = [executor.submit(run_clang_query, args.clang_query, query, args.extra_arg, path, define_args) for path, define_args in targets]
        for future in concurrent.futures.as_completed(futures):
            path, returncode, output = future.result()
            # 構文解析に失敗すると検出漏れになるので、エラーとして扱う
            if returncode != 0 or ERROR_PATTERN.search(output):
                print(f"::error file={path}::clang-query で解析できなかった", file=sys.stderr)
                print(output, file=sys.stderr)
                failed = True
                continue

            for line in output.splitlines():
                match = BIND_PATTERN.match(line)
                if match:
                    # clang-query は絶対パスで出力するので、リポジトリのトップからの相対パスにする
                    findings.add((os.path.relpath(match["file"], REPO_ROOT), int(match["line"]), match["kind"]))

    descriptions = {
        "call": "関数の 2 つ以上の引数で乱数を引いている",
        "ctor": "コンストラクタの 2 つ以上の引数で乱数を引いている",
        "binop": "二項演算子の両辺で乱数を引いている",
        "many": f"引数が {MAX_ARGS} 個を超える呼び出しの引数で乱数を引いている (引数の組を調べきれない)",
    }
    for file, line, kind in sorted(findings):
        print(f"::error file={file},line={line}::{descriptions[kind]}")
        print(f"{file}:{line}: {descriptions[kind]}")

    if findings:
        print()
        print("引数や両辺の評価順は規定されておらず、乱数を引く順序がコンパイラによって変わる。")
        print("コンストラクタは波括弧の初期化にし、それ以外は左から順に一時変数へ取り出して、引く順序を固定すること。")

    return 1 if findings or failed else 0


if __name__ == "__main__":
    sys.exit(main())
