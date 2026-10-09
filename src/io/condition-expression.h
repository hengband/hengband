#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <tl/optional.hpp>

/*!
 * @brief 条件式の変数の値を返す関数
 * @details 先頭の「$」を除いた変数名を受け取り、値を返す。知らない変数なら tl::nullopt を返す。
 */
using ExpressionVariableResolver = std::function<tl::optional<std::string>(std::string_view name)>;

enum class ConditionExpressionError {
    UNKNOWN_OPERATOR,
    UNKNOWN_VARIABLE,
    MISSING_CLOSING_BRACKET,
    INVALID_NUMBER,
};

tl::expected<std::string, ConditionExpressionError> evaluate_condition_expression_checked(std::string_view expr, const ExpressionVariableResolver &resolve);
std::string evaluate_condition_expression(std::string_view expr, const ExpressionVariableResolver &resolve);
