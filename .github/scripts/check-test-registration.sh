#!/bin/sh

# src/test 以下のテストソースが、autotools (src/Makefile.am) と
# MSVC (VisualStudio/Hengband/HengbandTest.vcxproj) の両方のビルド定義に
# 登録されているかをチェックする。
# automake も MSBuild もソースファイルを自動収集しないため、登録を忘れたテストは
# ビルドも実行もされない。それを検出する。

MAKEFILE_AM=src/Makefile.am
VCXPROJ=VisualStudio/Hengband/HengbandTest.vcxproj

# hengband_test_SOURCES への代入を、行継続とコメントを処理して
# 空白区切りの1行として取り出す
MAKEFILE_AM_SOURCES=$(awk '
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

# vcxproj の <ClCompile Include="..\..\src\..." /> から src/ 相対のパスを取り出す。
# Makefile.am 側で # コメントを除去しているのと揃えて、コメントアウトされた
# <ClCompile> を登録済みと誤判定しないよう、先に XML コメントを除去する。
# vcxproj は CRLF かつパスの区切りが \ なので、それぞれ除去・変換する
VCXPROJ_SOURCES=$(tr -d '\r' <$VCXPROJ |
    awk '
        {
            line = $0
            result = ""
            while (length(line) > 0) {
                if (in_comment) {
                    end = index(line, "-->")
                    if (end == 0) {
                        line = ""
                        break
                    }
                    line = substr(line, end + 3)
                    in_comment = 0
                } else {
                    start = index(line, "<!--")
                    if (start == 0) {
                        result = result line
                        break
                    }
                    result = result substr(line, 1, start - 1)
                    line = substr(line, start + 4)
                    in_comment = 1
                }
            }
            print result
        }
    ' |
    sed -n 's|.*<ClCompile Include="\([^"]*\)".*|\1|p' |
    tr '\\' '/' |
    sed -n 's|^\.\./\.\./src/||p' |
    tr '\n' ' ')

if [ -z "$MAKEFILE_AM_SOURCES" ]; then
    echo "$MAKEFILE_AM: hengband_test_SOURCES is not defined."
    exit 1
fi

# ClCompile が1つも取れないのは、パスの記法変更などでこのスクリプトが
# 追随できていない状態。黙って通さずエラーにする
if [ -z "$VCXPROJ_SOURCES" ]; then
    echo "$VCXPROJ: no <ClCompile> item was found."
    echo "If the project file has been moved or restructured, update this script."
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
    # どちらのビルド定義にも src/ からの相対パスで記述する
    RELATIVE_PATH=${file#src/}

    case " $MAKEFILE_AM_SOURCES " in
    *" $RELATIVE_PATH "*) ;;
    *)
        echo "$file: not registered in hengband_test_SOURCES of $MAKEFILE_AM."
        STATUS=1
        ;;
    esac

    case " $VCXPROJ_SOURCES " in
    *" $RELATIVE_PATH "*) ;;
    *)
        echo "$file: not registered as <ClCompile> in $VCXPROJ."
        STATUS=1
        ;;
    esac
done

if [ $STATUS -ne 0 ]; then
    echo ""
    echo "Add the test source files above to both build definitions:"
    echo "  - hengband_test_SOURCES in $MAKEFILE_AM"
    echo "  - <ClCompile> items in $VCXPROJ"
    echo "See src/test/README.md for details."
fi

exit $STATUS
