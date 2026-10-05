/*!
 * @brief 設定ファイルの条件式の解析のテスト
 *
 * io/pref-file-expressor.h の process_pref_file_expr() を検証する。
 * 構文の解析は evaluate_condition_expression() に任せているので、そのテストで確かめる。
 * 「$」で始まらない語や、語だけを使う条件式はプレイヤーの情報を使わないので、プレイヤーには nullptr を渡す。
 */

#include "io/pref-file-expressor.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

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

TEST_CASE("process_pref_file_expr returns an unknown marker for an unknown variable")
{
    CHECK(parse("$FOO") == "?o?o?");
}
