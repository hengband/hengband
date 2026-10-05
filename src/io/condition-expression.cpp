/*!
 * @brief 設定ファイルや町のマップの条件式の評価
 */

#include "io/condition-expression.h"
#include "system/h-basic.h"
#include "util/string-processor.h"
#include <cctype>
#include <cstdlib>
#include <cwctype>

namespace {

constexpr auto OPEN_BRACKET = '[';
constexpr auto CLOSE_BRACKET = ']';

/*!
 * @brief 語に含める文字かを返す
 * @details 空白と括弧以外の表示できる文字。日本語版では2バイト文字の前半バイトも含める
 */
bool is_word_char(char c)
{
#ifdef JP
    if (iskanji(c)) {
        return true;
    }
#endif
    return isprint(static_cast<unsigned char>(c)) && (std::string_view(" []").find(c) == std::string_view::npos);
}

/*!
 * @brief 条件式を先頭から1つずつ読み進めて評価する
 */
class ConditionExpressionEvaluator {
public:
    ConditionExpressionEvaluator(std::string_view expr, const ExpressionVariableResolver &resolver)
        : rest(expr)
        , resolve(resolver)
    {
    }
    ConditionExpressionEvaluator(const ConditionExpressionEvaluator &) = delete;
    ConditionExpressionEvaluator(ConditionExpressionEvaluator &&) = delete;
    ConditionExpressionEvaluator &operator=(const ConditionExpressionEvaluator &) = delete;
    ConditionExpressionEvaluator &operator=(ConditionExpressionEvaluator &&) = delete;

    std::string evaluate();

private:
    std::string_view rest; //!< まだ読んでいない部分
    char terminator = '\0'; //!< 直前に読んだ式の区切りの文字。入力の終わりなら '\0'
    const ExpressionVariableResolver &resolve;

    bool has_next_argument() const;
    void consume_terminator();
    template <typename Predicate>
    bool any_remaining_argument(Predicate pred);
    std::string evaluate_first_argument(const std::string &op);
    std::string evaluate_operator();
    std::string evaluate_word();
};

/*!
 * @brief 括弧で囲まれた式の中に、まだ引数が残っているかを返す
 */
bool ConditionExpressionEvaluator::has_next_argument() const
{
    return !this->rest.empty() && (this->terminator != CLOSE_BRACKET);
}

/*!
 * @brief 式の直後の1文字を区切りとして読む
 * @details 区切りの文字は空白・括弧・表示できない文字など何でもよい。入力の終わりなら何も読まない
 */
void ConditionExpressionEvaluator::consume_terminator()
{
    if (this->rest.empty()) {
        this->terminator = '\0';
        return;
    }

    this->terminator = this->rest.front();
    this->rest.remove_prefix(1);
}

/*!
 * @brief 式を1つ評価する
 * @return 評価の結果。括弧で囲まれた式なら "0" か "1"、語ならその語 (変数ならその値)。
 * 知らない演算子や変数なら "?o?o?"、括弧が閉じていなければ "?x?x?"
 */
std::string ConditionExpressionEvaluator::evaluate()
{
    while (!this->rest.empty() && iswspace(this->rest.front())) {
        this->rest.remove_prefix(1);
    }

    if (this->rest.starts_with(OPEN_BRACKET)) {
        this->rest.remove_prefix(1);
        return this->evaluate_operator();
    }

    return this->evaluate_word();
}

/*!
 * @brief 括弧で囲まれた式の残りの引数をすべて評価し、条件を満たすものがあったかを返す
 * @param pred 引数の評価の結果を受け取り、条件を満たすかを返す関数
 * @details 条件を満たす引数が見つかっても、閉じ括弧まで読み進めるために残りの引数も評価する
 */
template <typename Predicate>
bool ConditionExpressionEvaluator::any_remaining_argument(Predicate pred)
{
    auto found = false;
    while (this->has_next_argument()) {
        if (pred(this->evaluate())) {
            found = true;
        }
    }

    return found;
}

/*!
 * @brief 括弧で囲まれた式の最初の引数を評価する
 * @param op 演算子
 * @return 最初の引数の評価の結果。引数が無ければ演算子をそのまま返す
 */
std::string ConditionExpressionEvaluator::evaluate_first_argument(const std::string &op)
{
    return this->has_next_argument() ? this->evaluate() : op;
}

/*!
 * @brief 開き括弧の後ろの、演算子と引数を評価する
 */
std::string ConditionExpressionEvaluator::evaluate_operator()
{
    const auto op = this->evaluate();
    std::string v;
    if (op.empty()) {
        // 演算子が無ければ、引数を読まない
        v = "?o?o?";
    } else if (op == "IOR") {
        v = this->any_remaining_argument([](const auto &arg) { return !arg.empty() && (arg != "0"); }) ? "1" : "0";
    } else if (op == "AND") {
        v = this->any_remaining_argument([](const auto &arg) { return arg == "0"; }) ? "0" : "1";
    } else if (op == "NOT") {
        v = this->any_remaining_argument([](const auto &arg) { return arg == "1"; }) ? "0" : "1";
    } else if (op == "EQU") {
        const auto first = this->evaluate_first_argument(op);
        v = this->any_remaining_argument([&first](const auto &arg) { return arg == first; }) ? "1" : "0";
    } else if (op == "LEQ") {
        const auto first = atoi(this->evaluate_first_argument(op).data());
        v = this->any_remaining_argument([first](const auto &arg) { return !arg.empty() && (first > atoi(arg.data())); }) ? "0" : "1";
    } else if (op == "GEQ") {
        const auto first = atoi(this->evaluate_first_argument(op).data());
        v = this->any_remaining_argument([first](const auto &arg) { return !arg.empty() && (first < atoi(arg.data())); }) ? "0" : "1";
    } else {
        while (this->has_next_argument()) {
            this->evaluate();
        }
        v = "?o?o?";
    }

    if (this->terminator != CLOSE_BRACKET) {
        v = "?x?x?";
    }

    this->consume_terminator();
    return v;
}

/*!
 * @brief 語を1つ切り出して評価する
 * @details 2バイト文字は、後半バイトが空白や括弧と同じ値でも語に含める
 */
std::string ConditionExpressionEvaluator::evaluate_word()
{
    size_t length = 0;
    while ((length < this->rest.length()) && is_word_char(this->rest[length])) {
        length += is_multibyte_char_at(this->rest, length) ? 2 : 1;
    }

    const auto word = this->rest.substr(0, length);
    this->rest.remove_prefix(length);
    this->consume_terminator();
    if (!word.starts_with('$')) {
        return std::string(word);
    }

    return this->resolve(word.substr(1)).value_or("?o?o?");
}

}

/*!
 * @brief 条件式を評価する
 * @param expr 条件式。途中に NUL があれば、そこまでを条件式とする
 * @param resolve 「$」で始まる変数の値を返す関数
 * @return 評価の結果。括弧で囲まれた式なら "0" か "1"、語ならその語 (変数ならその値)。
 * 知らない演算子や変数なら "?o?o?"、括弧が閉じていなければ "?x?x?"。最初の式の後ろは読まない
 * @details
 * 書式は「[演算子 引数...]」か語で、引数にも式を書ける。演算子は次のとおり。
 * - IOR・AND・NOT: 空でない引数が "0" でないものがあるか・"0" のものがないか・"1" のものがないか
 * - EQU: 2番目以降の引数に、最初の引数と等しいものがあるか
 * - LEQ・GEQ: 最初の引数を数値として、空でない2番目以降の引数のすべて以下か・以上か
 */
std::string evaluate_condition_expression(std::string_view expr, const ExpressionVariableResolver &resolve)
{
    return ConditionExpressionEvaluator(expr.substr(0, expr.find('\0')), resolve).evaluate();
}
