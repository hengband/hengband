#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <tl/optional.hpp>

enum class ConditionExpressionError {
    UNKNOWN_OPERATOR,
    UNKNOWN_VARIABLE,
    MISSING_CLOSING_BRACKET,
    INVALID_NUMBER,
    INVALID_VARIABLE,
};

/*! @brief 条件式エラーと、変数の解決で失敗した場合の変数名（$を除く）。 */
struct ConditionExpressionFailure {
    ConditionExpressionError cause;
    std::string variable;
};

/*!
 * @brief 条件式の変数の値を返す関数
 * @details $を除いた変数名を受け取り、値を返す。未知の変数はnullopt、既知の変数の不正値はINVALID_VARIABLEを返す。
 */
using ExpressionVariableResult = tl::expected<tl::optional<std::string>, ConditionExpressionError>;
using ExpressionVariableResolver = std::function<ExpressionVariableResult(std::string_view name)>;

tl::optional<int> parse_condition_expression_integer(std::string_view text);
tl::expected<std::string, ConditionExpressionFailure> evaluate_condition_expression_detailed(std::string_view expr, const ExpressionVariableResolver &resolve);
tl::expected<std::string, ConditionExpressionError> evaluate_condition_expression_checked(std::string_view expr, const ExpressionVariableResolver &resolve);
std::string evaluate_condition_expression(std::string_view expr, const ExpressionVariableResolver &resolve);
