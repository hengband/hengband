#!/bin/sh

# src/test 以下のテストソースが src/Makefile.am の hengband_test_SOURCES に
# 登録されているかをチェックする。
# automake はソースファイルを自動収集しないため、登録を忘れたテストは
# ビルドも実行もされない。それを検出する。

MAKEFILE_AM=src/Makefile.am

# hengband_test_SOURCES への代入を、行継続とコメントを処理して
# 空白区切りの1行として取り出す
REGISTERED_SOURCES=$(awk '
    /^hengband_test_SOURCES[ \t]*\+?=/ { in_block = 1 }
    in_block {
        line = $0
        sub(/#.*/, "", line)
        print line
        if ($0 !~ /\\$/) {
            in_block = 0
        }
    }
' $MAKEFILE_AM | tr -s ' \t\n\\' ' ')

if [ -z "$REGISTERED_SOURCES" ]; then
    echo "$MAKEFILE_AM: hengband_test_SOURCES is not defined."
    exit 1
fi

TEST_SOURCES=$(find src/test -type f -name \*.cpp | sort)

# テストソースが1つも見つからないのは、ディレクトリの移動やリネームで
# このスクリプトが追随できていない状態。黙って通さずエラーにする
if [ -z "$TEST_SOURCES" ]; then
    echo "No test source file was found under src/test."
    echo "If the test directory has been moved, update this script."
    exit 1
fi

STATUS=0

# パスに空白が含まれても分割されないよう、単語分割を改行のみにする
IFS='
'

for file in $TEST_SOURCES; do
    # src/Makefile.am には src/ からの相対パスで記述する
    case " $REGISTERED_SOURCES " in
    *" ${file#src/} "*) ;;
    *)
        echo "$file: not registered in hengband_test_SOURCES of $MAKEFILE_AM."
        STATUS=1
        ;;
    esac
done

if [ $STATUS -ne 0 ]; then
    echo ""
    echo "Add the test source files above to hengband_test_SOURCES in $MAKEFILE_AM."
    echo "See src/test/README.md for details."
fi

exit $STATUS
