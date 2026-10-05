/*!
 * @brief 条件式の評価のテスト
 *
 * io/condition-expression.h の evaluate_condition_expression() を検証する。
 * 構文の解析の細かい動作は、これを使う process_pref_file_expr() のテストで確かめる。
 */

#include "io/condition-expression.h"

#include "system/h-basic.h"
#include "test/string-helpers.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

using namespace test;

namespace {

/*!
 * @brief 変数を解決しない関数
 */
tl::optional<std::string> resolve_nothing(std::string_view)
{
    return tl::nullopt;
}

/*!
 * @brief 条件式を、終端の NUL の無いバッファに置いて評価する
 * @param expr 条件式
 * @param resolve 変数の値を返す関数
 * @return 評価の結果
 * @details 文字列の終端を越えて読むと AddressSanitizer で検出できるよう、条件式だけが入る大きさのバッファに置く。
 */
std::string evaluate(std::string_view expr, const ExpressionVariableResolver &resolve = resolve_nothing)
{
    const std::vector<char> buf(expr.begin(), expr.end());
    return evaluate_condition_expression(std::string_view(buf.data(), buf.size()), resolve);
}

}

TEST_CASE("evaluate_condition_expression evaluates an expression")
{
    CHECK(evaluate("abc") == "abc");
    CHECK(evaluate("[EQU abc abc]") == "1");
    CHECK(evaluate("[AND [EQU a a] [NOT [EQU a b]]]") == "1");
    CHECK(evaluate("[LEQ 3 5]") == "1");
    CHECK(evaluate("[") == "?x?x?");
    CHECK(evaluate("") == "");
}

TEST_CASE("evaluate_condition_expression does not read past the end of the expression")
{
    CHECK(evaluate("[EQU a a") == "?x?x?");
    CHECK(evaluate("[EQU a a ") == "?x?x?");
    CHECK(evaluate("abc ") == "abc");
}

TEST_CASE("evaluate_condition_expression passes a variable name without the dollar sign")
{
    std::vector<std::string> names;
    const auto resolve = [&names](std::string_view name) -> tl::optional<std::string> {
        names.emplace_back(name);
        if (name == "FOO") {
            return "foo";
        }
        return tl::nullopt;
    };

    CHECK(evaluate("[EQU $FOO foo]", resolve) == "1");
    CHECK(evaluate("$BAR", resolve) == "?o?o?");
    CHECK(evaluate("$", resolve) == "?o?o?");
    CHECK(names == std::vector<std::string>{ "FOO", "BAR", "" });
}

TEST_CASE("evaluate_condition_expression uses a resolved value as is")
{
    const auto resolve = [](std::string_view) -> tl::optional<std::string> { return "a b]"; };
    CHECK(evaluate("$X", resolve) == "a b]");
    CHECK(evaluate("[EQU $X $Y]", resolve) == "1");
}

#ifdef JP
TEST_CASE("evaluate_condition_expression does not read past a word ending with a lone lead byte")
{
    const auto word = cat("ab", KANJI_KAN.substr(0, 1));
    CHECK(evaluate(word) == word);
    CHECK(evaluate(cat("[EQU ", word)) == "?x?x?");
}
#endif
