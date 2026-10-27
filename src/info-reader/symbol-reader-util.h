#pragma once

#include "info-reader/parse-error-types.h"
#include "view/display-symbol.h"
#include <nlohmann/json_fwd.hpp>
#include <tl/expected.hpp>

// 既存リーダーの型エラーと文字数・色の検証順序を保つ。
enum class SymbolReadPolicy {
    TYPE_AND_CHARACTER_FIRST,
    MONSTER_COLOR_FIRST,
};

tl::expected<DisplaySymbol, ParseErrorType> info_read_symbol(const nlohmann::json &json, SymbolReadPolicy policy = SymbolReadPolicy::TYPE_AND_CHARACTER_FIRST);
