/*!
 * @brief path_build() のテスト
 *
 * Windows 日本語版ではファイル名が Shift_JIS のため、std::filesystem::path へ
 * 渡す前に CP932 → UTF-16 へ変換する。相対パスだけでなく、'\\' 始まりの
 * 早期 return 経路でも同じ変換が必要なことを検証する。
 *
 * 2バイト文字のテストデータは必ず16進エスケープで書くこと。
 */

#include "util/angband-files.h"
#include <doctest/doctest.h>
#include <filesystem>
#include <string>
#include <string_view>

#if defined(_WIN32) && defined(JP) && defined(SJIS)
namespace {
template <typename... Args>
std::string cat(const Args &...args)
{
    std::string result;
    (result.append(args), ...);
    return result;
}

constexpr std::string_view KANJI_NI = "\x93\xfa"; //!< 日
constexpr std::string_view KANJI_HON = "\x96\x7b"; //!< 本
constexpr std::string_view DAME_SO = "\x83\x5c"; //!< ソ (後半バイトが 0x5c)
constexpr std::string_view UTF8_LOOKALIKE = "\xe0\xa0\x81\xe3\x82\x81"; //!< 燿√ａ

}

TEST_CASE("path_build appends a Shift_JIS file name as UTF-16 on Windows")
{
    const auto file = cat(KANJI_NI, KANJI_HON);
    const auto built = path_build(std::filesystem::path(L"pref"), file);
    CHECK(built.filename().wstring() == L"\u65e5\u672c");
}

TEST_CASE("path_build converts a root-relative Shift_JIS path on the early-return path")
{
    const auto file = cat("\\", KANJI_NI, KANJI_HON, "\\file.prf");
    const auto built = path_build(std::filesystem::path(L"ignored"), file);
    CHECK(built.wstring() == L"\\\u65e5\u672c\\file.prf");
}

TEST_CASE("path_build converts a Shift_JIS path whose second byte is 0x5c")
{
    const auto file = cat(DAME_SO, ".prf");
    const auto built = path_build(std::filesystem::path(L"pref"), file);
    CHECK(built.filename().wstring() == L"\u30bd.prf");
}

TEST_CASE("path_build converts a Shift_JIS path that is also valid UTF-8")
{
    const auto built = path_build(std::filesystem::path(L"pref"), std::string(UTF8_LOOKALIKE));
    CHECK(built.filename().wstring() == L"\u71ff\u221a\uff41");
}

TEST_CASE("path_build converts a Shift_JIS path when the directory argument is empty")
{
    const auto file = cat(KANJI_NI, KANJI_HON, ".prf");
    const auto built = path_build({}, file);
    CHECK(built.wstring() == L"\u65e5\u672c.prf");
}

#endif
