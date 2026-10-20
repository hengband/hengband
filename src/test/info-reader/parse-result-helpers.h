#pragma once

#include "info-reader/parse-error-types.h"
#include <tl/expected.hpp>

namespace test {

/*!
 * @brief 解析結果をエラーコードとして比べられるようにする
 * @param result 定義データの読み込み処理の解析結果
 * @return 成功時はPARSE_ERROR_NONE、失敗時は解析エラーコード
 */
inline parse_error_type to_error_code(const tl::expected<void, parse_error_type> &result)
{
    return result ? PARSE_ERROR_NONE : result.error();
}

}
