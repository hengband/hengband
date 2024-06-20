#include "locale/localized-string.h"

#ifdef JP
/*!
 * @brief コンストラクタ
 *
 * 日本語と英語の文字列を受け取り、それぞれの文字列を保持するLocalizedStringオブジェクトを生成する
 *
 * @param localized 日本語の文字列
 * @param english 英語の文字列
 * @note 引数を std::string_view にするとMSVCの警告C4868が出るので const std::string & にする
 */
LocalizedString::LocalizedString(const std::string &localized, const std::string &english)
    : localized(localized)
    , english(english)
{
}
#else
/*!
 * @brief コンストラクタ
 *
 * 日本語版とのコードの共通化のため日本語と英語の文字列を受け取るが、
 * 英語版では英語の文字列のみを保持するLocalizedStringオブジェクトを生成する
 *
 * @param english 英語の文字列
 */
LocalizedString::LocalizedString(const std::string &, const std::string &english)
    : english(english)
{
}
#endif

/*!
 * @brief メンバアクセス演算子
 *
 * 日本語版: 日本語の文字列へのアクセス演算子
 * 英語版: 英語の文字列へのアクセス演算子
 *
 * @return 文字列へのconstポインタ
 */
const std::string *LocalizedString::operator->() const noexcept
{
    return &this->string();
}

/*!
 * @brief std::string_view 型に変換する
 *
 * 日本語版: 日本語の文字列を std::string_view 型に変換する
 * 英語版: 英語の文字列を std::string_view 型に変換する
 *
 * @return 変換した std::string_view オブジェクト
 */
LocalizedString::operator std::string_view() const noexcept
{
    return this->string();
}

/*!
 * @brief 文字配列表現を取得する
 *
 * 日本語版: 日本語の文字列の文字配列表現
 * 英語版: 英語の文字列の文字配列表現
 *
 * @return 文字配列表現を指すconstポインタ
 * @note メンバアクセス演算子を使用して ->data() としても同じだが、
 * 使用頻度が高く std::string だった時代にすでに .data() が
 * 使われている箇所が多量にあるので互換性のために実装している
 */
const char *LocalizedString::data() const noexcept
{
    return this->string().data();
}

/*!
 * @brief 内部で保持している文字列への参照を取得する
 *
 * 日本語版: 日本語の文字列を取得する
 * 英語版: 英語の文字列を取得する
 *
 * @return 内部で保持している文字列へのconst参照を返す
 */
const std::string &LocalizedString::string() const noexcept
{
    return _(this->localized, this->english);
}

/*!
 * @brief 英語の文字列を取得する
 *
 * 日本語版: 英語の文字列を取得する
 * 英語版: 英語の文字列を取得する(string()と同じ)
 *
 * @return 内部で保持している文字列へのconst参照を返す
 */
const std::string &LocalizedString::en_string() const noexcept
{
    return this->english;
}

/*!
 * @brief 出力ストリームにLocalizeStringオブジェクトが保持する文字列を出力する
 *
 * 日本語版: 日本語の文字列を出力する
 * 英語版: 英語の文字列を出力する
 *
 * @param os 出力ストリーム
 * @param str 出力するLocalizedStringオブジェクト
 * @return std::ostream& 出力ストリーム
 */
std::ostream &operator<<(std::ostream &os, const LocalizedString &str)
{
    os << str.string();
    return os;
}
