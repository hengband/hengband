/*!
 * @brief 文字コード処理のテスト
 */

#include "locale/character-encoding.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>

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

constexpr std::string_view NIHON_EUC = "\xc6\xfc\xcb\xdc"; //!< 日本 (EUC-JP)
constexpr std::string_view NIHON_SJIS = "\x93\xfa\x96\x7b"; //!< 日本 (Shift_JIS)

#ifdef EUC
constexpr auto NIHON_SYS = NIHON_EUC;
#else
constexpr auto NIHON_SYS = NIHON_SJIS;
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

#endif
