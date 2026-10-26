/*!
 * @brief path_build() のテスト
 *
 * Windows 日本語版ではファイル名が Shift_JIS のため、std::filesystem::path へ
 * 渡す前に CP932 → UTF-16 へ変換する。相対パスだけでなく、'\\' 始まりの
 * 早期 return 経路でも同じ変換が必要なことを検証する。
 *
 * 2バイト文字のテストデータは16進エスケープで書く (src/test/README.md を参照)。
 *
 * また、Unix 版の path_parse() がパスの先頭の「~」を展開できない場合に、
 * 例外を投げず開けないパスとして扱うことを検証する。
 *
 * angband_fgets() は、'\0' を含む行をその '\0' で切った1行として返すことを検証する。
 */

#include "test/string-helpers.h"
#include "util/angband-files.h"
#include "util/finalizer.h"
#include <cstdio>
#include <doctest/doctest.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32) && defined(JP) && defined(SJIS)
using namespace test;

namespace {
constexpr std::string_view KANJI_NI = "\x93\xfa"; //!< 日
constexpr std::string_view KANJI_HON = "\x96\x7b"; //!< 本
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

#ifndef _WIN32
namespace {
//! 存在しないはずのユーザー名で始まるパス
constexpr auto PATH_OF_NO_SUCH_USER = "~hengband-test-no-such-user/file.txt";
}

TEST_CASE("path_parse returns an empty path for a user that does not exist")
{
    CHECK(path_parse(PATH_OF_NO_SUCH_USER).empty());
}

TEST_CASE("path_parse returns an empty path for a user name that is too long")
{
    const auto path = "~" + std::string(200, 'a') + "/file.txt";
    CHECK(path_parse(path).empty());
}

TEST_CASE("path_parse returns a path without a leading tilde as is")
{
    CHECK(path_parse("lib/help/help.hlp") == std::filesystem::path("lib/help/help.hlp"));
}

TEST_CASE("angband_fopen fails without throwing for a user that does not exist")
{
    CHECK(angband_fopen(PATH_OF_NO_SUCH_USER, FileOpenMode::READ) == nullptr);
}
#endif

using namespace std::literals;

namespace {
/*!
 * @brief 内容を一時ファイルに書き、angband_fgets() でファイルの終端まで読んだ行を返す
 * @param content ファイルの内容
 * @return 読んだ行の並び
 */
std::vector<std::string> read_all_lines(std::string_view content)
{
    auto *fp = std::tmpfile();
    REQUIRE(fp != nullptr);
    const auto close_file = util::make_finalizer([fp] { std::fclose(fp); });
    REQUIRE(std::fwrite(content.data(), 1, content.size(), fp) == content.size());
    std::rewind(fp);

    std::vector<std::string> lines;
    for (auto line = angband_fgets(fp); line; line = angband_fgets(fp)) {
        lines.push_back(std::move(*line));
    }

    return lines;
}
}

TEST_CASE("angband_fgets returns an empty line for a line starting with NUL")
{
    CHECK(read_all_lines("\0abc\nxyz\n"sv) == std::vector<std::string>{ "", "xyz" });

    // 改行の無い最後の行でも、読み取った行として空の行を返す
    CHECK(read_all_lines("\0abc"sv) == std::vector<std::string>{ "" });
}

TEST_CASE("angband_fgets returns an empty line for a long line starting with NUL")
{
    // 読み取りのバッファの大きさに関わらず、行全体を1行として扱う
    std::string content(1, '\0');
    content.append(1023, 'f').append("\nxyz\n");
    CHECK(read_all_lines(content) == std::vector<std::string>{ "", "xyz" });
}

TEST_CASE("angband_fgets cuts a line at NUL without joining it with the next line")
{
    CHECK(read_all_lines("ab\0cd\nxyz\n"sv) == std::vector<std::string>{ "ab", "xyz" });
}

TEST_CASE("angband_fgets keeps empty lines and the last line without a newline")
{
    CHECK(read_all_lines("abc\n\nxyz") == std::vector<std::string>{ "abc", "", "xyz" });
}
