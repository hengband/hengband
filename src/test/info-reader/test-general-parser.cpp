/*!
 * @brief 固定マップの各行の解析処理のテスト
 *
 * 建物の定義 (「B:」の行) を解析する parse_line_building のうち、行が途中で終わっている場合を検証する。
 * 正常な行は建物の一覧 (グローバルな配列) を書き換えるため、ここでは扱わない。
 */

#include "info-reader/general-parser.h"
#include "info-reader/parse-error-types.h"

#include <doctest/doctest.h>

#include <stdexcept>
#include <string_view>

TEST_CASE("parse_line_building reports too few arguments for a directive without arguments")
{
    // 指示子だけで終わっていて、その後の「:」と引数が無い (英語版の行は「B:」の後に「$」が付く)
    CHECK(parse_line_building(_("B:0:N", "B:$0:N")) == PARSE_ERROR_TOO_FEW_ARGUMENTS);
}

TEST_CASE("parse_line_building does not read past the end of the line after B:")
{
    // 「B:」で終わる行。view の外に「$」を置き、終端の先を読むと英語版の行の印と見誤るようにしておく
    constexpr std::string_view buffer = "B:$";
    const auto line = buffer.substr(0, 2);
#ifdef JP
    // 英語版の行とはみなさずに解析を続け、建物の番号が無いので数値の変換に失敗する
    CHECK_THROWS_AS(parse_line_building(line), std::invalid_argument);
#else
    // 英語版の行ではないので読み飛ばす
    CHECK(parse_line_building(line) == PARSE_ERROR_NONE);
#endif
}
