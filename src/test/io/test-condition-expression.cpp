/*!
 * @brief 条件式の評価のテスト
 *
 * io/condition-expression.h の evaluate_condition_expression() を検証する。
 * 変数を使わない構文の解析の動作は、ここで確かめる。
 */

#include "io/condition-expression.h"

#include "system/h-basic.h"
#include "test/string-helpers.h"

#include <doctest/doctest.h>

#include <limits>
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

TEST_CASE("evaluate_condition_expression does not read past the end of the expression")
{
    CHECK(evaluate("[EQU a a") == "0");
    CHECK(evaluate("[EQU a a ") == "0");
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
    CHECK(evaluate("$BAR", resolve) == "0");
    CHECK(evaluate("$", resolve) == "0");
    CHECK(names == std::vector<std::string>{ "FOO", "BAR", "" });
}

TEST_CASE("evaluate_condition_expression uses a resolved value as is")
{
    const auto resolve = [](std::string_view) -> tl::optional<std::string> { return "a b]"; };
    CHECK(evaluate("$X", resolve) == "a b]");
    CHECK(evaluate("[EQU $X $Y]", resolve) == "1");
}

TEST_CASE("evaluate_condition_expression reads only the first expression")
{
    CHECK(evaluate("  abc") == "abc");
    CHECK(evaluate("\t\n abc") == "abc");
    CHECK(evaluate("abc def") == "abc");
    CHECK(evaluate("a[b") == "a");
    CHECK(evaluate("a]b") == "a");
    CHECK(evaluate("[EQU a a] [EQU a b]") == "1");
}

TEST_CASE("evaluate_condition_expression stops at an embedded NUL")
{
    using namespace std::literals::string_view_literals;
    CHECK(evaluate("ab\0cd"sv) == "ab");
    CHECK(evaluate("[EQU a a\0]"sv) == "0");
}

TEST_CASE("evaluate_condition_expression evaluates logical operators")
{
    CHECK(evaluate("[IOR 0 0]") == "0");
    CHECK(evaluate("[IOR 0 1]") == "1");
    CHECK(evaluate("[IOR 0 abc]") == "1");
    CHECK(evaluate("[IOR]") == "0");
    CHECK(evaluate("[AND 1 1]") == "1");
    CHECK(evaluate("[AND 1 0]") == "0");
    CHECK(evaluate("[AND]") == "1");
    CHECK(evaluate("[NOT 0]") == "1");
    CHECK(evaluate("[NOT 1]") == "0");
    CHECK(evaluate("[NOT abc]") == "1");
}

TEST_CASE("evaluate_condition_expression evaluates nested expressions")
{
    CHECK(evaluate("[AND [EQU a a] [EQU b b]]") == "1");
    CHECK(evaluate("[AND [EQU a a] [EQU a b]]") == "0");
    CHECK(evaluate("[IOR [EQU a b] [NOT [EQU c d]]]") == "1");
    CHECK(evaluate("[NOT [IOR [EQU a b] [EQU c d]] ]") == "1");
}

TEST_CASE("evaluate_condition_expression compares words with EQU against the first argument")
{
    CHECK(evaluate("[EQU a b a]") == "1");
    CHECK(evaluate("[EQU a b c]") == "0");
    CHECK(evaluate("[EQU a]") == "0");
    CHECK(evaluate("[EQU]") == "0");
}

TEST_CASE("evaluate_condition_expression compares numbers with LEQ and GEQ")
{
    CHECK(evaluate("[LEQ 3 5]") == "1");
    CHECK(evaluate("[LEQ 5 3]") == "0");
    CHECK(evaluate("[LEQ 3 3 5]") == "1");
    CHECK(evaluate("[LEQ 3 5 2]") == "0");
    CHECK(evaluate("[LEQ 03 3]") == "1");
    CHECK(evaluate("[GEQ 5 3]") == "1");
    CHECK(evaluate("[GEQ 3 5]") == "0");
    CHECK(evaluate("[GEQ 3 3 1]") == "1");
    CHECK(evaluate("[LEQ 5]") == "1");
    CHECK(evaluate("[LEQ]") == "1");
    CHECK(evaluate("[GEQ]") == "1");
    CHECK(evaluate("[LEQ abc 1]") == "0");
    CHECK(evaluate("[GEQ abc 1]") == "0");
}

TEST_CASE("evaluate_condition_expression rejects invalid numeric arguments")
{
    const std::vector<std::string> invalid_values{
        "abc",
        "1tail",
        "+",
        "++1",
        "+-1",
        "-",
        "1.0",
        "0x1",
        std::to_string(std::numeric_limits<int>::max()) + "0",
        std::to_string(std::numeric_limits<int>::min()) + "0",
    };
    for (const auto &op : { "LEQ", "GEQ" }) {
        for (const auto &value : invalid_values) {
            CAPTURE(op);
            CAPTURE(value);
            CHECK(evaluate(cat("[", op, " ", value, " 0]")) == "0");
            CHECK(evaluate(cat("[", op, " 0 ", value, "]")) == "0");
            CHECK(evaluate(cat("[", op, " ", value, "]")) == "0");
        }
        CHECK(evaluate(cat("[", op, " $UNKNOWN 0]")) == "0");
        CHECK(evaluate(cat("[", op, " 0 $UNKNOWN]")) == "0");
    }
}

TEST_CASE("evaluate_condition_expression validates the entire resolved numeric value")
{
    using namespace std::literals::string_literals;
    for (const auto &value : { " 1"s, "1 "s, "\t1"s, "1\t"s, "1\0tail"s }) {
        CAPTURE(value);
        const auto resolve = [&value](std::string_view) -> tl::optional<std::string> { return value; };
        for (const auto &op : { "LEQ", "GEQ" }) {
            CHECK(evaluate(cat("[", op, " $X 0]"), resolve) == "0");
            CHECK(evaluate(cat("[", op, " 0 $X]"), resolve) == "0");
        }
    }
}

TEST_CASE("evaluate_condition_expression rejects invalid numbers throughout nested expressions")
{
    CHECK(evaluate("[NOT [LEQ abc 1]]") == "0");
    CHECK(evaluate("[NOT [GEQ abc 1]]") == "0");
    CHECK(evaluate("[IOR 1 [LEQ 0 abc]]") == "0");
    CHECK(evaluate("[AND 0 [GEQ 0 abc]]") == "0");
    CHECK(evaluate("[EQU [LEQ 0 abc] 1]") == "0");
    CHECK(evaluate("[LEQ 5 3 abc]") == "0");
    CHECK(evaluate("[NOT [LEQ 5 3 abc]]") == "0");
    CHECK(evaluate("[NOT [GEQ 3 5 abc]]") == "0");
    CHECK(evaluate("[LEQ [GEQ 0 abc] 1]") == "0");
    // 最初の式の後ろの不正な数値は評価対象に含めない
    CHECK(evaluate("[LEQ 0 1] [GEQ 0 abc]") == "1");
}

TEST_CASE("evaluate_condition_expression rejects all errors throughout nested expressions")
{
    // どの種類の評価エラーも、式全体を条件不成立とする。
    for (const auto &op : { "LEQ", "GEQ" }) {
        CAPTURE(op);
        CHECK(evaluate(cat("[FOO [", op, " abc 1]]")) == "0");
        CHECK(evaluate(cat("[", op, " abc 1")) == "0");
        // 数値が正常でも未知演算子・閉じ括弧不足を拒否する。
        CHECK(evaluate(cat("[FOO [", op, " 0 1]]")) == "0");
        CHECK(evaluate(cat("[", op, " 0 1")) == "0");
    }
}

TEST_CASE("evaluate_condition_expression preserves valid numeric and empty argument behavior")
{
    const auto min = std::to_string(std::numeric_limits<int>::min());
    const auto max = std::to_string(std::numeric_limits<int>::max());
    CHECK(evaluate(cat("[LEQ ", min, " ", max, "]")) == "1");
    CHECK(evaluate(cat("[GEQ ", max, " ", min, "]")) == "1");
    CHECK(evaluate(cat("[LEQ ", min, " ", min, "]")) == "1");
    CHECK(evaluate(cat("[GEQ ", max, " ", max, "]")) == "1");
    CHECK(evaluate("[LEQ -03 -2 -0]") == "1");
    CHECK(evaluate("[GEQ 03 2 -0]") == "1");
    CHECK(evaluate("[GEQ 5]") == "1");
    const auto resolve_empty = [](std::string_view) -> tl::optional<std::string> { return ""; };
    CHECK(evaluate("[LEQ $EMPTY 1]", resolve_empty) == "1");
    CHECK(evaluate("[GEQ $EMPTY -1]", resolve_empty) == "1");
    CHECK(evaluate("[LEQ $EMPTY -1]", resolve_empty) == "0");
    CHECK(evaluate("[GEQ $EMPTY 1]", resolve_empty) == "0");
    CHECK(evaluate("[LEQ 5 $EMPTY]", resolve_empty) == "1");
    CHECK(evaluate("[GEQ 5 $EMPTY]", resolve_empty) == "1");
}

TEST_CASE("evaluate_condition_expression preserves a leading plus on decimal integers")
{
    const auto max = std::to_string(std::numeric_limits<int>::max());
    CHECK(evaluate("[LEQ +03 3]") == "1");
    CHECK(evaluate("[GEQ 3 +03]") == "1");
    CHECK(evaluate("[LEQ +0 -0]") == "1");
    CHECK(evaluate(cat("[GEQ +", max, " ", max, "]")) == "1");
    CHECK(evaluate(cat("[LEQ 0 +", max, "0]")) == "0");
    const auto resolve = [](std::string_view) -> tl::optional<std::string> { return "+10"; };
    CHECK(evaluate("[LEQ $X +10]", resolve) == "1");
    CHECK(evaluate("[GEQ +10 $X]", resolve) == "1");
}

TEST_CASE("evaluate_condition_expression_checked distinguishes evaluation errors from false")
{
    const auto valid_false = evaluate_condition_expression_checked("[EQU a b]", resolve_nothing);
    REQUIRE(valid_false.has_value());
    CHECK(*valid_false == "0");
    const auto check_error = [](std::string_view expr, ConditionExpressionError expected) {
        CAPTURE(expr);
        const auto result = evaluate_condition_expression_checked(expr, resolve_nothing);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == expected);
        CHECK(evaluate(expr) == "0");
    };
    check_error("[LEQ abc 1]", ConditionExpressionError::INVALID_NUMBER);
    check_error("[FOO [LEQ abc 1]]", ConditionExpressionError::UNKNOWN_OPERATOR);
    check_error("[EQU a a", ConditionExpressionError::MISSING_CLOSING_BRACKET);
    check_error("$TYPO", ConditionExpressionError::UNKNOWN_VARIABLE);
    check_error("[IOR 1 [GEQ $TYPO 10]]", ConditionExpressionError::UNKNOWN_VARIABLE);
    check_error("[IOR 1 [EQU $TYPO x]]", ConditionExpressionError::UNKNOWN_VARIABLE);
    check_error("[NOT [EQU $TYPO x]]", ConditionExpressionError::UNKNOWN_VARIABLE);
    check_error("[IOR 1 [FOO a]]", ConditionExpressionError::UNKNOWN_OPERATOR);
    check_error("[NOT [FOO a]]", ConditionExpressionError::UNKNOWN_OPERATOR);
    check_error("[IOR 1 [EQU a a]", ConditionExpressionError::MISSING_CLOSING_BRACKET);
    // 変数の値がマーカーに似ていても、解決に成功していれば正当な語。
    const auto resolve = [](std::string_view) -> tl::optional<std::string> { return "?o?o?"; };
    const auto literal = evaluate_condition_expression_checked("[EQU $X ?o?o?]", resolve);
    REQUIRE(literal.has_value());
    CHECK(*literal == "1");
}

TEST_CASE("evaluate_condition_expression treats an empty word as an argument")
{
    // 表示できない文字は空白と違い読み飛ばされず、空の語の区切りになる
    CHECK(evaluate("\x01") == "");
    CHECK(evaluate("[EQU \x01\x01]") == "1");
    CHECK(evaluate("[EQU a \x01]") == "0");
    CHECK(evaluate("[IOR \x01]") == "0");
    CHECK(evaluate("[AND \x01]") == "1");
    CHECK(evaluate("[LEQ 5 \x01]") == "1");
    CHECK(evaluate("[GEQ 5 \x01]") == "1");
}

TEST_CASE("evaluate_condition_expression separates words with a tab")
{
    CHECK(evaluate("[EQU\ta\ta]") == "1");
    CHECK(evaluate("[EQU\ta\tb]") == "0");
}

TEST_CASE("evaluate_condition_expression rejects syntax errors")
{
    CHECK(evaluate("") == "");
    CHECK(evaluate("[]") == "0");
    CHECK(evaluate("[FOO a b]") == "0");
    CHECK(evaluate("[") == "0");
    CHECK(evaluate("[EQU a a") == "0");
    CHECK(evaluate("[AND [EQU a a]") == "0");
}

TEST_CASE("evaluate_condition_expression lets a closing bracket of an inner expression close the outer one")
{
    // 内側の式は閉じ括弧の次の1文字を区切りとして読むので、外側の閉じ括弧も読んでしまう
    CHECK(evaluate("[AND [EQU a a]]") == "1");
    CHECK(evaluate("[AND [EQU a a]] [EQU a b]") == "1");
    CHECK(evaluate("[AND [EQU a b]][EQU a a]") == "0");
    CHECK(evaluate("[NOT [EQU a b]]x 1]") == "1");
}

TEST_CASE("evaluate_condition_expression rejects an unknown variable")
{
    CHECK(evaluate("$") == "0");
    CHECK(evaluate("$FOO") == "0");
    CHECK(evaluate("[EQU $FOO ?o?o?]") == "0");
}

#ifdef JP
TEST_CASE("evaluate_condition_expression does not read past a word ending with a lone lead byte")
{
    const auto word = cat("ab", KANJI_KAN.substr(0, 1));
    CHECK(evaluate(word) == word);
    CHECK(evaluate(cat("[EQU ", word)) == "0");
}

TEST_CASE("evaluate_condition_expression reads a word containing a two-byte character")
{
    CHECK(evaluate(cat("ab", KANJI_KAN)) == cat("ab", KANJI_KAN));
    CHECK(evaluate(cat("[EQU ", KANJI_KAN, " ", KANJI_KAN, "]")) == "1");
}

TEST_CASE("evaluate_condition_expression reads a bracket after a lead byte as a trail byte")
{
    // 前半バイトの次の1バイトは、値にかかわらず後半バイトとして語に含める
    CHECK(evaluate(cat("[EQU a", KANJI_KAN.substr(0, 1), "]")) == "0");
    CHECK(evaluate(cat("[EQU a", KANJI_KAN.substr(0, 1), "] a]")) == "0");
}
#endif

#if defined(JP) && defined(SJIS)
TEST_CASE("evaluate_condition_expression reads a two-byte character whose trail byte is a bracket")
{
    CHECK(evaluate(cat("[EQU ", DAME_KANA_ZO, " ", DAME_KANA_ZO, "]")) == "1");
    CHECK(evaluate(cat("[EQU ", DAME_CHOON, " ", DAME_CHOON, "]")) == "1");
    CHECK(evaluate(cat("a", DAME_KANA_ZO, "b")) == cat("a", DAME_KANA_ZO, "b"));
}
#endif
