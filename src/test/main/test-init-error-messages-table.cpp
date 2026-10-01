/*!
 * @brief ゲームデータ解析エラー名の表のテスト
 */

#include "main/init-error-messages-table.h"

#include <doctest/doctest.h>

TEST_CASE("err_str has a message for every parse error")
{
    // PARSE_ERROR_NONE (0) はエラーではないので名称を持たない
    for (auto err = 1; err < PARSE_ERROR_MAX; err++) {
        CAPTURE(err);
        CHECK(err_str[err] != nullptr);
    }
}
