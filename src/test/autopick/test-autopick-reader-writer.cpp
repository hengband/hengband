/*!
 * @brief 自動拾い設定ファイルの読み書きのテスト
 */

#include "autopick/autopick-reader-writer.h"

#include "autopick/autopick-util.h"
#include "io/files-util.h"
#include "test/scoped-restore.h"
#include "test/temporary-json-files.h"

#include <doctest/doctest.h>
#include <fmt/format.h>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr auto PLAYER_BASE_NAME = "Tester";

//! 以前の行数の上限 (5998 行) を超える行数
constexpr auto MANY_LINES = 7000;

/*!
 * @brief 「line1」から始まる番号付きの行を作る
 * @param num_lines 行数
 * @return 行の配列
 */
std::vector<std::string> make_numbered_lines(int num_lines)
{
    std::vector<std::string> lines;
    for (auto i = 1; i <= num_lines; i++) {
        lines.push_back(fmt::format("line{}", i));
    }

    return lines;
}

/*!
 * @brief 行の配列を改行でつなげる
 * @param lines 行の配列
 * @return 各行の末尾に改行を付けてつなげた文字列
 */
std::string join_lines(const std::vector<std::string> &lines)
{
    std::string text;
    for (const auto &line : lines) {
        text.append(line).append("\n");
    }

    return text;
}

}

TEST_CASE("read_pickpref_text_lines reads all the lines of a file with many lines")
{
    test::TemporaryJsonFiles files;
    const auto restore = test::scoped_restore(ANGBAND_DIR_USER);
    ANGBAND_DIR_USER = files.directory;
    const auto expected = make_numbered_lines(MANY_LINES);
    files.write_raw(pickpref_filename(PLAYER_BASE_NAME, PickType::WITH_PNAME), join_lines(expected));

    const auto [filename_mode, lines] = read_pickpref_text_lines(PLAYER_BASE_NAME);
    CHECK(filename_mode == PickType::WITH_PNAME);
    CHECK(lines == expected);
}

TEST_CASE("read_pickpref_text_lines reads an existing empty file without falling back to another file")
{
    test::TemporaryJsonFiles files;
    const auto restore = test::scoped_restore(ANGBAND_DIR_USER);
    ANGBAND_DIR_USER = files.directory;
    files.write_raw(pickpref_filename(PLAYER_BASE_NAME, PickType::WITH_PNAME), "");
    files.write_raw(pickpref_filename(PLAYER_BASE_NAME, PickType::DEFAULT), "other\n");

    const auto [filename_mode, lines] = read_pickpref_text_lines(PLAYER_BASE_NAME);
    CHECK(filename_mode == PickType::WITH_PNAME);
    CHECK(lines.empty());
}

TEST_CASE("write_text_lines writes all the lines of many lines")
{
    test::TemporaryJsonFiles files;
    const auto restore = test::scoped_restore(ANGBAND_DIR_USER);
    ANGBAND_DIR_USER = files.directory;
    const auto lines = make_numbered_lines(MANY_LINES);

    REQUIRE(write_text_lines("written.prf", lines));
    // 改行は環境によって変わる (Windows では CRLF) ので、書いたときと同じテキストモードで読む
    std::ifstream ifs(files.directory / "written.prf");
    std::vector<std::string> written_lines;
    std::string line;
    while (std::getline(ifs, line)) {
        written_lines.push_back(std::move(line));
    }

    CHECK(written_lines == lines);
}
