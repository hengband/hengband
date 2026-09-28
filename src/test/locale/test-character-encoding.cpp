/*!
 * @brief 文字コード処理のテスト
 */

#include "locale/character-encoding.h"

#include <doctest/doctest.h>

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

#ifdef EUC
constexpr auto NIHON_SYS = NIHON_EUC;
constexpr auto KANJI_SEN_SYS = KANJI_SEN_EUC;
constexpr auto KANJI_SHAKU_SYS = KANJI_SHAKU_EUC;
constexpr auto KANJI_YOU_SYS = KANJI_YOU_EUC;
constexpr auto KANJI_SEN2_SYS = KANJI_SEN2_EUC;
constexpr std::string_view FULLWIDTH_TILDE_SYS = "\xa1\xc1"; //!< ～ (EUC-JP では波ダッシュに置き換える)
constexpr std::string_view FULLWIDTH_HYPHEN_MINUS_SYS = "\xa1\xdd"; //!< － (EUC-JP ではマイナス記号に置き換える)
#else
constexpr auto NIHON_SYS = NIHON_SJIS;
constexpr auto KANJI_SEN_SYS = KANJI_SEN_SJIS;
constexpr auto KANJI_SHAKU_SYS = KANJI_SHAKU_SJIS;
constexpr auto KANJI_YOU_SYS = KANJI_YOU_SJIS;
constexpr auto KANJI_SEN2_SYS = KANJI_SEN2_SJIS;
constexpr std::string_view FULLWIDTH_TILDE_SYS = "\x81\x60"; //!< ～ (CP932)
constexpr std::string_view FULLWIDTH_HYPHEN_MINUS_SYS = "\x81\x7c"; //!< － (CP932)
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

TEST_CASE("utf8_to_local converts fullwidth tilde and hyphen-minus")
{
    CHECK(utf8_to_local(cat(FULLWIDTH_TILDE_UTF8, FULLWIDTH_HYPHEN_MINUS_UTF8)) == cat(FULLWIDTH_TILDE_SYS, FULLWIDTH_HYPHEN_MINUS_SYS));
}

#ifdef EUC

TEST_CASE("utf8_to_euc converts fullwidth tilde and hyphen-minus to wave dash and minus sign")
{
    auto utf8 = cat(FULLWIDTH_TILDE_UTF8, FULLWIDTH_HYPHEN_MINUS_UTF8);
    char euc[16]{};
    REQUIRE(utf8_to_euc(utf8.data(), utf8.length() + 1, euc, sizeof(euc)) >= 0);
    CHECK(std::string_view(euc) == cat(FULLWIDTH_TILDE_SYS, FULLWIDTH_HYPHEN_MINUS_SYS));
}

TEST_CASE("utf8_to_euc converts fullwidth tilde after an embedded NUL within the given length")
{
    // 途中に '\0' があっても、渡した長さの範囲はすべて置き換えてから変換する
    auto utf8 = cat("a\0"sv, FULLWIDTH_TILDE_UTF8);
    char euc[16]{};
    REQUIRE(utf8_to_euc(utf8.data(), utf8.length() + 1, euc, sizeof(euc)) == 5);
    CHECK(std::string_view(euc, 4) == "a\0\xa1\xc1"sv);
}

TEST_CASE("utf8_to_euc does not read beyond the terminator of a truncated UTF-8 character")
{
    constexpr std::pair<std::string_view, std::string_view> truncated_chars[] = {
        { "E3", "\xe3"sv },
        { "E3 81", "\xe3\x81"sv },
        { "F0 9F", "\xf0\x9f"sv },
        { "F0 9F 98", "\xf0\x9f\x98"sv },
    };

    for (const auto &[label, truncated] : truncated_chars) {
        CAPTURE(label);

        // 終端の後ろに全角チルダを置き、終端を越えて読むとそれが置き換えられることで検出する。
        // どの位置から読み進めても全角チルダの先頭に当たるよう、3つ続けて置く
        const auto original = cat(truncated, "\0"sv, FULLWIDTH_TILDE_UTF8, FULLWIDTH_TILDE_UTF8, FULLWIDTH_TILDE_UTF8);
        auto buf = original;
        char euc[16]{};
        utf8_to_euc(buf.data(), truncated.length() + 1, euc, sizeof(euc));
        CHECK(buf == original);
    }
}

TEST_CASE("utf8_to_sys replaces JIS X 0212 characters with question marks")
{
    // é (UTF-8 で2バイト) は EUC-JP では JIS X 0212 の3バイト (8F AB B1) になり、正しく表示できないので '?' に置き換える
    CHECK(utf8_to_sys("caf\xc3\xa9"sv) == "caf?"sv);
    CHECK(utf8_to_sys("\xc3\xa9\xc3\xa9\xc3\xa9"sv) == "???"sv);
    CHECK(utf8_to_sys("\xe6\x97\xa5\xe6\x9c\xac\xc3\xa9"sv) == cat(NIHON_EUC, "?"));
}

TEST_CASE("utf8_to_euc returns the length after replacing JIS X 0212 characters")
{
    auto utf8 = cat("a\xc3\xa9"sv, "b");
    char euc[16]{};
    CHECK(utf8_to_euc(utf8.data(), utf8.length() + 1, euc, sizeof(euc)) == 4);
    CHECK(std::string_view(euc) == "a?b");
}

TEST_CASE("utf8_to_euc converts JIS X 0212 characters into a buffer as long as the input")
{
    // '?' に置き換えた後の長さで足りれば、置き換える前の3バイトが入らない大きさのバッファでも変換できる
    auto utf8 = cat("\xc3\xa9\xc3\xa9\xc3\xa9"sv);
    std::vector<char> euc(utf8.length() + 1);
    CHECK(utf8_to_euc(utf8.data(), utf8.length() + 1, euc.data(), euc.size()) == 4);
    CHECK(std::string_view(euc.data()) == "???");
}

TEST_CASE("utf8_to_local replaces JIS X 0212 characters with question marks")
{
    CHECK(utf8_to_local("caf\xc3\xa9"sv) == "caf?"sv);
}

TEST_CASE("utf8_to_local throws on invalid UTF-8")
{
    CHECK_THROWS_AS(utf8_to_local("\xff"sv), std::runtime_error);
}

#endif

#endif
