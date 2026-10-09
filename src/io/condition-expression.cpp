/*!
 * @brief 設定ファイルや町のマップの条件式の評価
 */

#include "io/condition-expression.h"
#include "system/h-basic.h"
#include "util/string-processor.h"
#include <cctype>
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

    tl::expected<std::string, ConditionExpressionError> evaluate();

private:
    std::string_view rest; //!< まだ読んでいない部分
    char terminator = '\0'; //!< 直前に読んだ式の区切りの文字。入力の終わりなら '\0'
    const ExpressionVariableResolver &resolve;
    tl::optional<ConditionExpressionError> error; //!< 最初に検出した評価エラー

    void record_error(ConditionExpressionError cause);
    std::string evaluate_next();
    int parse_number(std::string_view arg);
    bool has_next_argument() const;
    void consume_terminator();
    template <typename Predicate>
    bool any_remaining_argument(Predicate pred);
    std::string evaluate_first_argument();
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
 * 評価エラーは別途記録し、最初の式全体の評価後に呼び出し側へ返す
 */
std::string ConditionExpressionEvaluator::evaluate_next()
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
 * @brief 最初の条件式の評価結果か、検出したエラーを返す
 * @details 内側の式のエラーも NOT や IOR で真に変換せず呼び出し側へ返す
 */
tl::expected<std::string, ConditionExpressionError> ConditionExpressionEvaluator::evaluate()
{
    auto result = this->evaluate_next();
    if (this->error) {
        return tl::unexpected(*this->error);
    }
    return result;
}

void ConditionExpressionEvaluator::record_error(ConditionExpressionError cause)
{
    if (!this->error) {
        this->error = cause;
    }
}

/*!
 * @brief 数値比較の引数全体を int の範囲の10進整数として変換する
 * @details 空の先頭引数は従来どおり0とする。先頭の「+」1文字は従来の設定との互換性のため許容する
 */
int ConditionExpressionEvaluator::parse_number(std::string_view arg)
{
    if (arg.empty()) {
        return 0;
    }

    if (arg.starts_with('+')) {
        arg.remove_prefix(1);
        if (arg.empty() || arg.starts_with('-') || arg.starts_with('+')) {
            this->record_error(ConditionExpressionError::INVALID_NUMBER);
            return 0;
        }
    }
    const auto number = str_to_num<int>(arg);
    if (!number) {
        this->record_error(ConditionExpressionError::INVALID_NUMBER);
    }
    return number.value_or(0);
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
        if (pred(this->evaluate_next())) {
            found = true;
        }
    }

    return found;
}

/*!
 * @brief 括弧で囲まれた式の最初の引数を評価する
 * @return 最初の引数の評価の結果。引数が無ければ空文字列
 */
std::string ConditionExpressionEvaluator::evaluate_first_argument()
{
    return this->has_next_argument() ? this->evaluate_next() : "";
}

/*!
 * @brief 開き括弧の後ろの、演算子と引数を評価する
 */
std::string ConditionExpressionEvaluator::evaluate_operator()
{
    const auto op = this->evaluate_next();
    std::string v;
    if (op.empty()) {
        // 演算子が無ければ、引数を読まない
        this->record_error(ConditionExpressionError::UNKNOWN_OPERATOR);
    } else if (op == "IOR") {
        v = this->any_remaining_argument([](const auto &arg) { return !arg.empty() && (arg != "0"); }) ? "1" : "0";
    } else if (op == "AND") {
        v = this->any_remaining_argument([](const auto &arg) { return arg == "0"; }) ? "0" : "1";
    } else if (op == "NOT") {
        v = this->any_remaining_argument([](const auto &arg) { return arg == "1"; }) ? "0" : "1";
    } else if (op == "EQU") {
        const auto first = this->evaluate_first_argument();
        v = this->any_remaining_argument([&first](const auto &arg) { return arg == first; }) ? "1" : "0";
    } else if (op == "LEQ") {
        const auto first = this->parse_number(this->evaluate_first_argument());
        v = this->any_remaining_argument([this, first](const auto &arg) { return !arg.empty() && (first > this->parse_number(arg)); }) ? "0" : "1";
    } else if (op == "GEQ") {
        const auto first = this->parse_number(this->evaluate_first_argument());
        v = this->any_remaining_argument([this, first](const auto &arg) { return !arg.empty() && (first < this->parse_number(arg)); }) ? "0" : "1";
    } else {
        this->record_error(ConditionExpressionError::UNKNOWN_OPERATOR);
        while (this->has_next_argument()) {
            this->evaluate_next();
        }
    }

    if (this->terminator != CLOSE_BRACKET) {
        this->record_error(ConditionExpressionError::MISSING_CLOSING_BRACKET);
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

    const auto value = this->resolve(word.substr(1));
    if (!value) {
        this->record_error(ConditionExpressionError::UNKNOWN_VARIABLE);
    }
    return value.value_or("");
}

}

/*!
 * @brief 条件式を評価し、評価結果とエラーを区別して返す
 * @param expr 条件式。途中に NUL があれば、そこまでを条件式とする
 * @param resolve 「$」で始まる変数の値を返す関数
 * @return 評価の結果、または最初に検出した ConditionExpressionError。最初の式の後ろは読まない
 * @details
 * 書式は「[演算子 引数...]」か語で、引数にも式を書ける。演算子は次のとおり。
 * - IOR・AND・NOT: 空でない引数が "0" でないものがあるか・"0" のものがないか・"1" のものがないか
 * - EQU: 2番目以降の引数に、最初の引数と等しいものがあるか
 * - LEQ・GEQ: 最初の引数を数値として、空でない2番目以降の引数のすべて以下か・以上か
 * LEQ・GEQ の数値は int の範囲の10進整数とし、負数と先頭の0を許す。既存設定との互換性のため先頭の「+」1文字も許す。
 * 空白や末尾の余分な文字は許さない。
 * 引数が無ければ真、空の先頭引数は0、空の後続引数は比較しない。
 * 不正な数値・未知の演算子・未知の変数・閉じ括弧不足は、演算子や入れ子の位置によらず式全体のエラーとする。
 * LEQ/GEQ に渡る未知変数も UNKNOWN_VARIABLE として返し、数値不正と区別する。
 */
tl::expected<std::string, ConditionExpressionError> evaluate_condition_expression_checked(std::string_view expr, const ExpressionVariableResolver &resolve)
{
    return ConditionExpressionEvaluator(expr.substr(0, expr.find('\0')), resolve).evaluate();
}

/*!
 * @brief 条件判定用に評価結果を返す。すべての評価エラーは条件不成立の "0" とする
 * @details 書式は evaluate_condition_expression_checked() を参照。エラーの通知が必要な呼び出し側は checked 版を使う
 */
std::string evaluate_condition_expression(std::string_view expr, const ExpressionVariableResolver &resolve)
{
    return evaluate_condition_expression_checked(expr, resolve).value_or("0");
}
