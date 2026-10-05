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
    CHECK(evaluate("[EQU a a\0]"sv) == "?x?x?");
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
    CHECK(evaluate("[LEQ abc 1]") == "1");
    CHECK(evaluate("[GEQ abc 1]") == "0");
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

TEST_CASE("evaluate_condition_expression returns error markers")
{
    CHECK(evaluate("") == "");
    CHECK(evaluate("[]") == "?o?o?");
    CHECK(evaluate("[FOO a b]") == "?o?o?");
    CHECK(evaluate("[") == "?x?x?");
    CHECK(evaluate("[EQU a a") == "?x?x?");
    CHECK(evaluate("[AND [EQU a a]") == "?x?x?");
}

TEST_CASE("evaluate_condition_expression lets a closing bracket of an inner expression close the outer one")
{
    // 内側の式は閉じ括弧の次の1文字を区切りとして読むので、外側の閉じ括弧も読んでしまう
    CHECK(evaluate("[AND [EQU a a]]") == "1");
    CHECK(evaluate("[AND [EQU a a]] [EQU a b]") == "1");
    CHECK(evaluate("[AND [EQU a b]][EQU a a]") == "0");
    CHECK(evaluate("[NOT [EQU a b]]x 1]") == "1");
}

TEST_CASE("evaluate_condition_expression returns an unknown marker for an unknown variable")
{
    CHECK(evaluate("$") == "?o?o?");
    CHECK(evaluate("$FOO") == "?o?o?");
    CHECK(evaluate("[EQU $FOO ?o?o?]") == "1");
}

#ifdef JP
TEST_CASE("evaluate_condition_expression does not read past a word ending with a lone lead byte")
{
    const auto word = cat("ab", KANJI_KAN.substr(0, 1));
    CHECK(evaluate(word) == word);
    CHECK(evaluate(cat("[EQU ", word)) == "?x?x?");
}

TEST_CASE("evaluate_condition_expression reads a word containing a two-byte character")
{
    CHECK(evaluate(cat("ab", KANJI_KAN)) == cat("ab", KANJI_KAN));
    CHECK(evaluate(cat("[EQU ", KANJI_KAN, " ", KANJI_KAN, "]")) == "1");
}

TEST_CASE("evaluate_condition_expression reads a bracket after a lead byte as a trail byte")
{
    // 前半バイトの次の1バイトは、値にかかわらず後半バイトとして語に含める
    CHECK(evaluate(cat("[EQU a", KANJI_KAN.substr(0, 1), "]")) == "?x?x?");
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
