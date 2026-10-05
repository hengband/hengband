#pragma once

/*!
 * @brief 文字列を扱うテストで共通に使うヘルパーと、2バイト文字のテストデータ
 *
 * 2バイト文字のテストデータは必ず16進エスケープで書くこと。
 * 日本語版のビルドはソースの文字列リテラルを変換する (autotoolsは gcc-wrap が nkf で
 * EUC-JPへ、MSVCは /execution-charset:shift-jis でShift_JISへ) が、英語版では変換されない。
 * ソースに日本語をそのまま書くと、ビルド構成によってバイト列が変わってしまう。
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

#if defined(JP) && defined(SJIS)
inline constexpr std::string_view KANJI_KAN = "\x8a\xbf"; //!< 漢
inline constexpr std::string_view KANJI_JI = "\x8e\x9a"; //!< 字
#elif defined(JP)
inline constexpr std::string_view KANJI_KAN = "\xb4\xc1"; //!< 漢
inline constexpr std::string_view KANJI_JI = "\xbb\xfa"; //!< 字
#endif

// ダメ文字 (Shift_JIS で後半バイトが ASCII の文字と同じ値になる2バイト文字)
#if defined(JP) && defined(SJIS)
inline constexpr std::string_view DAME_SO = "\x83\x5c"; //!< ソ (後半バイトが 0x5c、ASCIIの '\')
inline constexpr std::string_view DAME_HYOU = "\x95\x5c"; //!< 表 (後半バイトが 0x5c、ASCIIの '\')
inline constexpr std::string_view DAME_KANA_A = "\x83\x41"; //!< ア (後半バイトが 0x41、ASCIIの 'A')
inline constexpr std::string_view DAME_KANA_DI = "\x83\x61"; //!< ヂ (後半バイトが 0x61、ASCIIの 'a')
inline constexpr std::string_view DAME_KANA_TA = "\x83\x5e"; //!< タ (後半バイトが 0x5e、ASCIIの '^')
inline constexpr std::string_view DAME_KANA_ZO = "\x83\x5d"; //!< ゾ (後半バイトが 0x5d、ASCIIの ']')
inline constexpr std::string_view DAME_CHOON = "\x81\x5b"; //!< ー (後半バイトが 0x5b、ASCIIの '[')
inline constexpr std::string_view DAME_SPACE = "\x81\x40"; //!< 全角スペース (後半バイトが 0x40、ASCIIの '@')
#endif

// UTF-8としても正しいバイト列になる2バイト文字の並び。バイト数とコードポイント数が異なる
#if defined(JP) && defined(SJIS)
inline constexpr std::string_view UTF8_LOOKALIKE = "\xe0\xa0\x81\xe3\x82\x81"; //!< 燿√ａ (UTF-8では U+0801 U+3081 の2コードポイント)
inline constexpr std::string_view UTF8_LOOKALIKE_1ST = "\xe0\xa0"; //!< 燿
inline constexpr std::string_view UTF8_LOOKALIKE_2ND = "\x81\xe3"; //!< √
#elif defined(JP)
inline constexpr std::string_view UTF8_LOOKALIKE = "\xc2\xa3\xcd\xbf"; //!< 贈与 (UTF-8では U+00A3 U+037F の2コードポイント)
inline constexpr std::string_view UTF8_LOOKALIKE_1ST = "\xc2\xa3"; //!< 贈
inline constexpr std::string_view UTF8_LOOKALIKE_2ND = "\xcd\xbf"; //!< 与
#endif

}
