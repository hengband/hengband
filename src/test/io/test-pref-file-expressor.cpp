/*!
 * @brief 設定ファイルの条件式の解析のテスト
 *
 * io/pref-file-expressor.h の process_pref_file_expr() を検証する。
 * 「$」で始まらない語や、語だけを使う条件式はプレイヤーの情報を使わないので、プレイヤーには nullptr を渡す。
 */

#include "io/pref-file-expressor.h"

#include "system/h-basic.h"
#include "test/string-helpers.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

using namespace test;

namespace {

/*!
 * @brief 条件式を1つ解析する
 * @param expr 条件式
 * @return 解析の結果
 * @details 文字列の終端を越えて読むと AddressSanitizer で検出できるよう、
 * 条件式だけが入る大きさの (終端の NUL の無い) バッファに置いてから解析する。
 */
std::string parse(std::string_view expr)
{
    const std::vector<char> buf(expr.begin(), expr.end());
    return process_pref_file_expr(nullptr, std::string_view(buf.data(), buf.size()));
}

}

TEST_CASE("process_pref_file_expr returns a plain word as is")
{
    CHECK(parse("abc") == "abc");
}

TEST_CASE("process_pref_file_expr compares words with EQU")
{
    CHECK(parse("[EQU abc abc]") == "1");
    CHECK(parse("[EQU abc abd]") == "0");
}

TEST_CASE("process_pref_file_expr reads only the first expression")
{
    CHECK(parse("  abc") == "abc");
    CHECK(parse("\t\n abc") == "abc");
    CHECK(parse("abc def") == "abc");
    CHECK(parse("a[b") == "a");
    CHECK(parse("a]b") == "a");
    CHECK(parse("[EQU a a] [EQU a b]") == "1");
}

TEST_CASE("process_pref_file_expr stops at an embedded NUL")
{
    using namespace std::literals::string_view_literals;
    CHECK(parse("ab\0cd"sv) == "ab");
    CHECK(parse("[EQU a a\0]"sv) == "?x?x?");
}

TEST_CASE("process_pref_file_expr evaluates logical operators")
{
    CHECK(parse("[IOR 0 0]") == "0");
    CHECK(parse("[IOR 0 1]") == "1");
    CHECK(parse("[IOR 0 abc]") == "1");
    CHECK(parse("[IOR]") == "0");
    CHECK(parse("[AND 1 1]") == "1");
    CHECK(parse("[AND 1 0]") == "0");
    CHECK(parse("[AND]") == "1");
    CHECK(parse("[NOT 0]") == "1");
    CHECK(parse("[NOT 1]") == "0");
    CHECK(parse("[NOT abc]") == "1");
}

TEST_CASE("process_pref_file_expr evaluates nested expressions")
{
    CHECK(parse("[AND [EQU a a] [EQU b b]]") == "1");
    CHECK(parse("[AND [EQU a a] [EQU a b]]") == "0");
    CHECK(parse("[IOR [EQU a b] [NOT [EQU c d]]]") == "1");
    CHECK(parse("[NOT [IOR [EQU a b] [EQU c d]] ]") == "1");
}

TEST_CASE("process_pref_file_expr compares words with EQU against the first argument")
{
    CHECK(parse("[EQU a b a]") == "1");
    CHECK(parse("[EQU a b c]") == "0");
    CHECK(parse("[EQU a]") == "0");
    CHECK(parse("[EQU]") == "0");
}

TEST_CASE("process_pref_file_expr compares numbers with LEQ and GEQ")
{
    CHECK(parse("[LEQ 3 5]") == "1");
    CHECK(parse("[LEQ 5 3]") == "0");
    CHECK(parse("[LEQ 3 3 5]") == "1");
    CHECK(parse("[LEQ 3 5 2]") == "0");
    CHECK(parse("[LEQ 03 3]") == "1");
    CHECK(parse("[GEQ 5 3]") == "1");
    CHECK(parse("[GEQ 3 5]") == "0");
    CHECK(parse("[GEQ 3 3 1]") == "1");
    CHECK(parse("[LEQ 5]") == "1");
    CHECK(parse("[LEQ]") == "1");
    CHECK(parse("[GEQ]") == "1");
    CHECK(parse("[LEQ abc 1]") == "1");
    CHECK(parse("[GEQ abc 1]") == "0");
}

TEST_CASE("process_pref_file_expr treats an empty word as an argument")
{
    // 表示できない文字は空白と違い読み飛ばされず、空の語の区切りになる
    CHECK(parse("\x01") == "");
    CHECK(parse("[EQU \x01\x01]") == "1");
    CHECK(parse("[EQU a \x01]") == "0");
    CHECK(parse("[IOR \x01]") == "0");
    CHECK(parse("[AND \x01]") == "1");
    CHECK(parse("[LEQ 5 \x01]") == "1");
    CHECK(parse("[GEQ 5 \x01]") == "1");
}

TEST_CASE("process_pref_file_expr separates words with a tab")
{
    CHECK(parse("[EQU\ta\ta]") == "1");
    CHECK(parse("[EQU\ta\tb]") == "0");
}

TEST_CASE("process_pref_file_expr returns error markers")
{
    CHECK(parse("") == "");
    CHECK(parse("[]") == "?o?o?");
    CHECK(parse("[FOO a b]") == "?o?o?");
    CHECK(parse("[") == "?x?x?");
    CHECK(parse("[EQU a a") == "?x?x?");
    CHECK(parse("[AND [EQU a a]") == "?x?x?");
}

TEST_CASE("process_pref_file_expr lets a closing bracket of an inner expression close the outer one")
{
    // 内側の式は閉じ括弧の次の1文字を区切りとして読むので、外側の閉じ括弧も読んでしまう
    CHECK(parse("[AND [EQU a a]]") == "1");
    CHECK(parse("[AND [EQU a a]] [EQU a b]") == "1");
    CHECK(parse("[AND [EQU a b]][EQU a a]") == "0");
    CHECK(parse("[NOT [EQU a b]]x 1]") == "1");
}

TEST_CASE("process_pref_file_expr returns an unknown marker for an unknown variable")
{
    CHECK(parse("$") == "?o?o?");
    CHECK(parse("$FOO") == "?o?o?");
    CHECK(parse("[EQU $FOO ?o?o?]") == "1");
}

#ifdef JP
TEST_CASE("process_pref_file_expr reads a word containing a two-byte character")
{
    CHECK(parse(cat("ab", KANJI_KAN)) == cat("ab", KANJI_KAN));
    CHECK(parse(cat("[EQU ", KANJI_KAN, " ", KANJI_KAN, "]")) == "1");
}

TEST_CASE("process_pref_file_expr does not read past a word ending with a lone lead byte")
{
    const auto word = cat("ab", KANJI_KAN.substr(0, 1));
    CHECK(parse(word) == word);
}

TEST_CASE("process_pref_file_expr reads a bracket after a lead byte as a trail byte")
{
    // 前半バイトの次の1バイトは、値にかかわらず後半バイトとして語に含める
    CHECK(parse(cat("[EQU a", KANJI_KAN.substr(0, 1), "]")) == "?x?x?");
    CHECK(parse(cat("[EQU a", KANJI_KAN.substr(0, 1), "] a]")) == "0");
}
#endif

#if defined(JP) && defined(SJIS)
TEST_CASE("process_pref_file_expr reads a two-byte character whose trail byte is a bracket")
{
    CHECK(parse(cat("[EQU ", DAME_KANA_ZO, " ", DAME_KANA_ZO, "]")) == "1");
    CHECK(parse(cat("[EQU ", DAME_CHOON, " ", DAME_CHOON, "]")) == "1");
    CHECK(parse(cat("a", DAME_KANA_ZO, "b")) == cat("a", DAME_KANA_ZO, "b"));
}
#endif
