/*!
 * @brief 文字コード処理のテスト
 */

#include "locale/character-encoding.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <utility>

#ifdef JP

namespace {

/*
 * 日本語版のビルドでは文字列リテラルの文字コードが変換されるため、2バイト文字はエスケープで書く。
 * 16進エスケープは後続の英数字まで取り込んでしまうため、ASCII は別の文字列として cat() で連結する。
 */
template <typename... Args>
std::string cat(const Args &...args)
{
    std::string result;
    (result.append(args), ...);
    return result;
}

using namespace std::string_view_literals;

constexpr std::string_view NIHON_EUC = "\xc6\xfc\xcb\xdc"; //!< 日本 (EUC-JP)
constexpr std::string_view NIHON_SJIS = "\x93\xfa\x96\x7b"; //!< 日本 (Shift_JIS)

/*
 * 第2水準漢字 (1バイト目が E0 以上)
 * 燹 の EUC-JP と 爍 の Shift_JIS は同じバイト列 (E0 A1) で、どちらの文字コードとも解釈できる
 */
constexpr std::string_view KANJI_SEN_EUC = "\xe0\xa1"; //!< 燹 (EUC-JP、Shift_JIS とも解釈できる)
constexpr std::string_view KANJI_SEN_SJIS = "\xe0\x9f"; //!< 燹 (Shift_JIS、2バイト目が 80-A0 なので Shift_JIS に限られる)
constexpr std::string_view KANJI_SHAKU_EUC = "\xe0\xa3"; //!< 爍 (EUC-JP、Shift_JIS とも解釈できる)
constexpr std::string_view KANJI_SHAKU_SJIS = "\xe0\xa1"; //!< 爍 (Shift_JIS、EUC-JP とも解釈できる)
constexpr std::string_view KANJI_YOU_EUC = "\xe0\xfe"; //!< 珱 (EUC-JP、2バイト目が FD 以上なので EUC-JP に限られる)
constexpr std::string_view KANJI_YOU_SJIS = "\xe0\xfc"; //!< 珱 (Shift_JIS、EUC-JP とも解釈できる)
constexpr std::string_view KANJI_SEN2_EUC = "\xf0\xa1"; //!< 陝 (EUC-JP、1バイト目が F0 以上なので EUC-JP に限られる)
constexpr std::string_view KANJI_SEN2_SJIS = "\xe8\x9f"; //!< 陝 (Shift_JIS)

constexpr std::string_view FULLWIDTH_TILDE_UTF8 = "\xef\xbd\x9e"; //!< ～ (U+FF5E)
constexpr std::string_view FULLWIDTH_HYPHEN_MINUS_UTF8 = "\xef\xbc\x8d"; //!< － (U+FF0D)
constexpr std::string_view HALFWIDTH_KATAKANA_UTF8 = "\xef\xbd\xb1\xef\xbd\xb2\xef\xbd\xb3"; //!< ｱｲｳ (UTF-8 では1文字3バイト)

#ifdef EUC
constexpr auto NIHON_SYS = NIHON_EUC;
constexpr auto KANJI_SEN_SYS = KANJI_SEN_EUC;
constexpr auto KANJI_SHAKU_SYS = KANJI_SHAKU_EUC;
constexpr auto KANJI_YOU_SYS = KANJI_YOU_EUC;
constexpr auto KANJI_SEN2_SYS = KANJI_SEN2_EUC;
constexpr std::string_view FULLWIDTH_TILDE_SYS = "\xa1\xc1"; //!< ～ (EUC-JP では波ダッシュに置き換える)
constexpr std::string_view FULLWIDTH_HYPHEN_MINUS_SYS = "\xa1\xdd"; //!< － (EUC-JP ではマイナス記号に置き換える)
constexpr std::string_view HALFWIDTH_KATAKANA_SYS = "\x8e\xb1\x8e\xb2\x8e\xb3"; //!< ｱｲｳ (EUC-JP)
#else
constexpr auto NIHON_SYS = NIHON_SJIS;
constexpr auto KANJI_SEN_SYS = KANJI_SEN_SJIS;
constexpr auto KANJI_SHAKU_SYS = KANJI_SHAKU_SJIS;
constexpr auto KANJI_YOU_SYS = KANJI_YOU_SJIS;
constexpr auto KANJI_SEN2_SYS = KANJI_SEN2_SJIS;
constexpr std::string_view FULLWIDTH_TILDE_SYS = "\x81\x60"; //!< ～ (CP932)
constexpr std::string_view FULLWIDTH_HYPHEN_MINUS_SYS = "\x81\x7c"; //!< － (CP932)
constexpr std::string_view HALFWIDTH_KATAKANA_SYS = "\xb1\xb2\xb3"; //!< ｱｲｳ (CP932 では1文字1バイト)
#endif

}

TEST_CASE("codeconv detects EUC-JP followed by ASCII")
{
    auto str = cat(NIHON_EUC, "abc");
    CHECK(codeconv(str.data()) == CharacterEncoding::EUC_JP);
    CHECK(str == cat(NIHON_SYS, "abc"));
}

TEST_CASE("codeconv detects Shift_JIS followed by ASCII")
{
    auto str = cat(NIHON_SJIS, "abc");
    CHECK(codeconv(str.data()) == CharacterEncoding::SHIFT_JIS);
    CHECK(str == cat(NIHON_SYS, "abc"));
}

TEST_CASE("codeconv detects EUC-JP surrounded by ASCII")
{
    auto str = cat("abc", NIHON_EUC, "def");
    CHECK(codeconv(str.data()) == CharacterEncoding::EUC_JP);
    CHECK(str == cat("abc", NIHON_SYS, "def"));
}

TEST_CASE("codeconv rejects EUC-JP and Shift_JIS mixed with ASCII between them")
{
    const auto original = cat(NIHON_EUC, "abc", NIHON_SJIS);
    auto str = original;
    CHECK(codeconv(str.data()) == CharacterEncoding::UNKNOWN);
    CHECK(str == original);
}

TEST_CASE("codeconv detects Shift_JIS whose second byte is 0x80-0xA0 after a lead byte of 0xE0 or above")
{
    auto str = cat(KANJI_SEN_SJIS, "abc");
    CHECK(codeconv(str.data()) == CharacterEncoding::SHIFT_JIS);
    CHECK(str == cat(KANJI_SEN_SYS, "abc"));
}

TEST_CASE("codeconv detects EUC-JP whose second byte is 0xFD or above after a lead byte of 0xE0 or above")
{
    auto str = cat(KANJI_YOU_EUC, "abc");
    CHECK(codeconv(str.data()) == CharacterEncoding::EUC_JP);
    CHECK(str == cat(KANJI_YOU_SYS, "abc"));
}

TEST_CASE("codeconv defers ambiguous characters and decides by a following Shift_JIS character")
{
    auto str = cat(KANJI_SHAKU_SJIS, KANJI_YOU_SJIS, KANJI_SEN_SJIS);
    CHECK(codeconv(str.data()) == CharacterEncoding::SHIFT_JIS);
    CHECK(str == cat(KANJI_SHAKU_SYS, KANJI_YOU_SYS, KANJI_SEN_SYS));
}

TEST_CASE("codeconv defers ambiguous characters and decides by a following EUC-JP character")
{
    auto str = cat(KANJI_SEN_EUC, KANJI_SHAKU_EUC, NIHON_EUC);
    CHECK(codeconv(str.data()) == CharacterEncoding::EUC_JP);
    CHECK(str == cat(KANJI_SEN_SYS, KANJI_SHAKU_SYS, NIHON_SYS));
}

TEST_CASE("codeconv detects EUC-JP whose lead byte is 0xF0 or above")
{
    auto str = cat(KANJI_SEN2_EUC, "abc");
    CHECK(codeconv(str.data()) == CharacterEncoding::EUC_JP);
    CHECK(str == cat(KANJI_SEN2_SYS, "abc"));
}

TEST_CASE("codeconv does not treat Shift_JIS user-defined characters as Shift_JIS")
{
    // 1バイト目が F0 以上の Shift_JIS は sjis2euc() で EUC-JP に変換できないため、変換しない
    const auto original = cat(NIHON_SJIS, "\xf0\x40");
    auto str = original;
    CHECK(codeconv(str.data()) == CharacterEncoding::UNKNOWN);
    CHECK(str == original);
}

TEST_CASE("codeconv treats lead byte 0xF0 or above with second byte 0xA1 or above as EUC-JP")
{
    // Shift_JIS のユーザー定義文字とも読めるが EUC-JP とみなすので、後に Shift_JIS が続くと壊れた文字列になる
    const auto original = cat(KANJI_SEN2_EUC, KANJI_SEN2_SJIS);
    auto str = original;
    CHECK(codeconv(str.data()) == CharacterEncoding::UNKNOWN);
    CHECK(str == original);
}

TEST_CASE("codeconv does not treat Shift_JIS IBM extended characters as EUC-JP")
{
    // 髙 (Shift_JIS の IBM 拡張文字 FB FC) の1バイト目は EUC-JP の JIS X 0208 の範囲外なので、EUC-JP とみなさない
    const auto original = cat("\xfb\xfc", KANJI_SEN_EUC);
    auto str = original;
    CHECK(codeconv(str.data()) == CharacterEncoding::UNKNOWN);
    CHECK(str == original);
}

TEST_CASE("codeconv returns UNKNOWN for string with ambiguous characters only")
{
    const auto original = cat(KANJI_SEN_EUC, "abc", KANJI_SHAKU_EUC);
    auto str = original;
    CHECK(codeconv(str.data()) == CharacterEncoding::UNKNOWN);
    CHECK(str == original);
}

TEST_CASE("codeconv returns UNKNOWN for string ending with a lead byte")
{
    const auto original = cat(NIHON_EUC, "\xc6");
    auto str = original;
    CHECK(codeconv(str.data()) == CharacterEncoding::UNKNOWN);
    CHECK(str == original);
}

TEST_CASE("codeconv returns UNKNOWN for ASCII-only string")
{
    std::string str = "abc";
    CHECK(codeconv(str.data()) == CharacterEncoding::UNKNOWN);
    CHECK(str == "abc");
}

TEST_CASE("codeconv returns UNKNOWN for empty string")
{
    std::string str;
    CHECK(codeconv(str.data()) == CharacterEncoding::UNKNOWN);
}

TEST_CASE("utf8_to_sys converts fullwidth tilde and hyphen-minus")
{
    CHECK(utf8_to_sys(cat(FULLWIDTH_TILDE_UTF8, FULLWIDTH_HYPHEN_MINUS_UTF8)) == cat(FULLWIDTH_TILDE_SYS, FULLWIDTH_HYPHEN_MINUS_SYS));
}

TEST_CASE("sys_to_utf8 converts half-width katakana")
{
    CHECK(sys_to_utf8(HALFWIDTH_KATAKANA_SYS) == HALFWIDTH_KATAKANA_UTF8);
}

TEST_CASE("sys_to_utf8 converts an empty string to an empty string")
{
    CHECK(sys_to_utf8(""sv) == ""sv);
}

TEST_CASE("utf8_to_sys rejects a string with an embedded NUL")
{
    // 途中の '\0' より後ろが黙って欠けないよう、不正な入力として変換しない
    CHECK_FALSE(utf8_to_sys("a\0b"sv).has_value());
}

#ifdef EUC

TEST_CASE("utf8_to_euc converts fullwidth tilde and hyphen-minus to wave dash and minus sign")
{
    CHECK(utf8_to_euc(cat(FULLWIDTH_TILDE_UTF8, FULLWIDTH_HYPHEN_MINUS_UTF8)) == cat(FULLWIDTH_TILDE_SYS, FULLWIDTH_HYPHEN_MINUS_SYS));
}

TEST_CASE("utf8_to_euc converts fullwidth tilde after an embedded NUL")
{
    // 途中に '\0' があっても、文字列の長さの範囲はすべて置き換えてから変換する
    CHECK(utf8_to_euc(cat("a\0"sv, FULLWIDTH_TILDE_UTF8)) == "a\0\xa1\xc1"sv);
}

TEST_CASE("utf8_to_euc and utf8_to_sys convert an empty string to an empty string")
{
    CHECK(utf8_to_euc(""sv) == ""sv);
    CHECK(utf8_to_sys(""sv) == ""sv);
}

TEST_CASE("utf8_to_euc rejects a truncated UTF-8 character")
{
    constexpr std::pair<std::string_view, std::string_view> truncated_chars[] = {
        { "E3", "\xe3"sv },
        { "E3 81", "\xe3\x81"sv },
        { "F0 9F", "\xf0\x9f"sv },
        { "F0 9F 98", "\xf0\x9f\x98"sv },
    };

    for (const auto &[label, truncated] : truncated_chars) {
        CAPTURE(label);

        // 文字の途中で終わる文字列は、黙って捨てずに変換の失敗とする
        CHECK_FALSE(utf8_to_euc(truncated).has_value());
        CHECK_FALSE(utf8_to_euc(cat(truncated, "\0"sv)).has_value());
    }
}

TEST_CASE("utf8_to_sys replaces JIS X 0212 characters with question marks")
{
    // é (UTF-8 で2バイト) は EUC-JP では JIS X 0212 の3バイト (8F AB B1) になり、正しく表示できないので '?' に置き換える
    CHECK(utf8_to_sys("caf\xc3\xa9"sv) == "caf?"sv);
    CHECK(utf8_to_sys("\xc3\xa9\xc3\xa9\xc3\xa9"sv) == "???"sv);
    CHECK(utf8_to_sys("\xe6\x97\xa5\xe6\x9c\xac\xc3\xa9"sv) == cat(NIHON_EUC, "?"));
}

TEST_CASE("utf8_to_euc replaces JIS X 0212 characters with question marks")
{
    CHECK(utf8_to_euc(cat("a\xc3\xa9"sv, "b")) == "a?b"sv);
}

TEST_CASE("utf8_to_sys rejects invalid UTF-8")
{
    CHECK_FALSE(utf8_to_sys("\xff"sv).has_value());
}

#endif

#else

TEST_CASE("utf8_to_sys returns the input as is in the English version")
{
    CHECK(utf8_to_sys(std::string_view("abc")) == std::string_view("abc"));
}

#endif
