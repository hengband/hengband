#!/usr/bin/env python3
"""
C++ の #include ディレクティブで "" と <> が適切に使われているかチェックする。

ルール:
  - 変愚蛮怒のソースヘッダ (src/ 配下、external-lib/ を除く) は ""
  - 外部ライブラリのヘッダ (src/external-lib/include/ 配下) は <>
  - システムヘッダは <>
"""

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent.parent
SRC = REPO_ROOT / "src"
EXTERNAL_LIB_INCLUDE = SRC / "external-lib" / "include"

INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*([<"])(.*?)([>"])')

# ./configure で生成されるヘッダ。ソースツリーに存在しないためチェック対象外とする
GENERATED_HEADERS = {"autoconf.h"}


def check_file(path: Path) -> int:
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return 0

    rel_path = path.relative_to(REPO_ROOT)
    error_count = 0

    for lineno, line in enumerate(lines, 1):
        m = INCLUDE_PATTERN.match(line)
        if not m:
            continue

        open_q, include_path = m.group(1), m.group(2)

        if include_path in GENERATED_HEADERS:
            continue

        candidate = SRC / include_path
        is_hengband_src = candidate.is_file() and not candidate.is_relative_to(
            EXTERNAL_LIB_INCLUDE
        )

        if open_q == "<":
            # src/ 配下かつ external-lib/ 外のヘッダに <> を使っていたらエラー
            if is_hengband_src:
                print(
                    f'{rel_path}:{lineno}: error: use "" for Hengband source header:'
                    f" #include <{include_path}>"
                )
                error_count += 1
        else:
            # "" は変愚ソースヘッダ (src/ 配下かつ external-lib/include/ 外) にのみ使用可
            if not is_hengband_src:
                if (EXTERNAL_LIB_INCLUDE / include_path).is_file():
                    msg = "use <> for external library header"
                else:
                    msg = "use <> for system/non-project header"
                print(f'{rel_path}:{lineno}: error: {msg}: #include "{include_path}"')
                error_count += 1

    return error_count


def main() -> int:
    error_count = 0
    for path in sorted(SRC.rglob("*.cpp")) + sorted(SRC.rglob("*.h")):
        # src/external-lib/ 配下のファイル自体はチェック対象外（サードパーティコード）
        try:
            path.relative_to(SRC / "external-lib")
            continue
        except ValueError:
            pass

        error_count += check_file(path)

    if error_count > 0:
        print(f"\n{error_count} include style error(s) found.")
        return 1

    print("Include directive style check: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
