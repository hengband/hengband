/*!
 * @brief 生い立ちの編集で、行の表示桁を扱う処理のテスト
 *
 * 2バイト文字のテストデータは16進エスケープで書く (src/test/README.md を参照)。
 */

#include "birth/history-editor.h"

#include "test/string-helpers.h"

#include <doctest/doctest.h>
#include <string>

using namespace test;

TEST_CASE("history_char_start_column returns the column itself for single-byte characters")
{
    CHECK(history_char_start_column("abc", 0) == 0);
    CHECK(history_char_start_column("abc", 2) == 2);
}

TEST_CASE("history_char_start_column returns the line width for a column past the end of the line")
{
    CHECK(history_char_start_column("abc", 5) == 3);
}

TEST_CASE("history_char_at_column returns the character at the column")
{
    CHECK(history_char_at_column("abc", 1) == "b");
    CHECK(history_char_at_column("abc", 3).empty());
}

TEST_CASE("overwrite_history_char replaces a single-byte character")
{
    std::string line = "abc";
    overwrite_history_char(line, 1, "x");
    CHECK(line == "axc");
}

TEST_CASE("overwrite_history_char appends a character at the end of the line")
{
    std::string line = "abc";
    overwrite_history_char(line, 3, "x");
    CHECK(line == "abcx");
}

#ifdef JP
TEST_CASE("history_char_start_column moves the right half of a wide character to its start")
{
    const auto line = cat("a", KANJI_KAN, "b");
    CHECK(history_char_start_column(line, 1) == 1);
    CHECK(history_char_start_column(line, 2) == 1);
    CHECK(history_char_start_column(line, 3) == 3);
}

TEST_CASE("history_char_at_column returns a wide character as a whole")
{
    const auto line = cat("a", KANJI_KAN, "b");
    CHECK(history_char_at_column(line, 1) == KANJI_KAN);
    CHECK(history_char_at_column(line, 2) == KANJI_KAN);
    CHECK(history_char_at_column(line, 3) == "b");
}

TEST_CASE("overwrite_history_char fills the rest of an overwritten wide character with a space")
{
    auto line = cat("a", KANJI_KAN, "b");
    overwrite_history_char(line, 1, "x");
    CHECK(line == "ax b");
}

TEST_CASE("overwrite_history_char writes a wide character over two single-byte characters")
{
    std::string line = "abcd";
    overwrite_history_char(line, 1, KANJI_KAN);
    CHECK(line == cat("a", KANJI_KAN, "d"));
}

TEST_CASE("overwrite_history_char keeps the width when a wide character overlaps the next wide character")
{
    auto line = cat("a", "b", KANJI_JI, "c");
    overwrite_history_char(line, 1, KANJI_KAN);
    CHECK(line == cat("a", KANJI_KAN, " c"));
}

TEST_CASE("overwrite_history_char keeps the width when a wide character is written at the last two columns")
{
    std::string line = "abcd";
    overwrite_history_char(line, 2, KANJI_KAN);
    CHECK(line == cat("ab", KANJI_KAN));
}

TEST_CASE("overwrite_history_char replaces a wide character with a wide character")
{
    auto line = cat("a", KANJI_JI, "b");
    overwrite_history_char(line, 1, KANJI_KAN);
    CHECK(line == cat("a", KANJI_KAN, "b"));
}
#endif
