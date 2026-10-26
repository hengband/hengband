#pragma once

#include "system/angband.h"
#include <string>
#include <string_view>
#include <vector>

/*!
 * @brief 2 つの文字の間で行を分けられるか
 */
enum class LineBreakKind {
    NONE, //!< 分けられない
    WORD_BREAK, //!< 語の切れ目 (前後のどちらかが全角文字か、前が空白) で分けられる
    MID_WORD, //!< 語の途中 (半角文字どうしの間) で分けられる
};

LineBreakKind classify_line_break(std::string_view prev, std::string_view next);
bool needs_line_break_backtrack(std::string_view prev, std::string_view next);
std::vector<std::string> shape_buffer(std::string_view sv, size_t maxlen);
