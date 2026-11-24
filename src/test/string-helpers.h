#pragma once

/*!
 * @brief 文字列を扱うテストで共通に使うヘルパーと、多バイト文字のテストデータ
 *
 * 多バイト文字のテストデータは必ず16進エスケープで書くこと。
 * 日本語版ではソースの文字列リテラルは変換されずに UTF-8 のままビルドされるが、旧版の文字コード (EUC-JP・Shift_JIS) のバイト列は
 * 文字では書けないので、どの文字コードのデータも16進エスケープで書き、期待するバイト列を明示する。
 */

#include <string>
#include <string_view>

namespace test {

/*!
 * @brief テストデータの文字列を連結する
 *
 * 16進エスケープは後続の文字まで貪欲に取り込むため ("\x83\x5c" の直後に "A" を
 * 隣接させると "\x5cA" と解釈される)、リテラルの隣接連結を使わずにこの関数で連結する。
 */
template <typename... Args>
std::string cat(const Args &...args)
{
    std::string result;
    (result.append(args), ...);
    return result;
}

/*!
 * @brief 文字列を指定回数繰り返す
 */
inline std::string repeat(std::string_view str, int count)
{
    std::string result;
    for (auto i = 0; i < count; ++i) {
        result.append(str);
    }

    return result;
}

#if defined(JP) && defined(UTF8)
inline constexpr std::string_view KANJI_KAN = "\xe6\xbc\xa2"; //!< 漢
inline constexpr std::string_view KANJI_JI = "\xe5\xad\x97"; //!< 字
inline constexpr std::string_view KAKKO_OPEN = "\xe3\x80\x8c"; //!< 「 (行末禁則文字)
#endif

}
