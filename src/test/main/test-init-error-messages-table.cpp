/*!
 * @brief ゲームデータ解析エラー名の表のテスト
 */

#include "info-reader/parse-error-types.h"
#include "main/init-error-messages-table.h"
#include "util/enum-converter.h"
#include <doctest/doctest.h>

TEST_CASE("err_str has a message for every parse error")
{
    // ParseErrorType::NONE (0) はエラーではないので名称を持たない
    for (auto err = 1; err < enum2i(ParseErrorType::MAX); err++) {
        CAPTURE(err);
        CHECK(err_str[err] != nullptr);
    }
}
