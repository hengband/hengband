/*!
 * @brief 設定ファイルの条件式の解析のテスト
 *
 * io/pref-file-expressor.h の process_pref_file_expr() を検証する。
 * 「$」で始まらない語や、語だけを使う条件式はプレイヤーの情報を使わないので、プレイヤーには nullptr を渡す。
 */

#include "io/pref-file-expressor.h"

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
 * 条件式と終端の NUL だけが入る大きさのバッファに置いてから解析する。
 */
std::string parse(std::string_view expr)
{
    std::vector<char> buf(expr.begin(), expr.end());
    buf.push_back('\0');
    auto *s = buf.data();
    char f;
    return process_pref_file_expr(nullptr, &s, &f);
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
#endif
