/*!
 * @brief 自動拾いエディタの編集操作のテスト
 */

#include "autopick/autopick-editor-util.h"

#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-editor-command.h"
#include "autopick/autopick-inserter-killer.h"
#include "autopick/autopick-util.h"

#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <string_view>

namespace {

//! is_greater_autopick_max_line() が偽になる最大の行数
constexpr auto MAX_EDITABLE_LINES = MAX_LINES - 2;

/*!
 * @brief 空でない行を指定の行数だけ持つエディタの状態を作る
 * @param num_lines 行数
 * @return エディタの状態
 */
text_body_type make_text_body(int num_lines)
{
    text_body_type tb(0, 0);
    tb.lines_list.resize(MAX_LINES);
    for (auto y = 0; y < num_lines; y++) {
        tb.lines_list[y] = std::make_unique<std::string>("a");
    }

    return tb;
}

/*!
 * @brief 1行だけを持ち、カーソルがその行にあるエディタの状態を作る
 * @param line 行の内容
 * @param cx カーソルの位置 (バイト)
 * @return エディタの状態
 */
text_body_type make_text_body_with_line(std::string_view line, int cx)
{
    auto tb = make_text_body(1);
    *tb.lines_list[0] = line;
    tb.cx = cx;
    return tb;
}

#if defined(JP) && defined(SJIS)
constexpr std::string_view KANJI_KAN = "\x8a\xbf"; //!< 漢
constexpr std::string_view KANJI_JI = "\x8e\x9a"; //!< 字
#elif defined(JP)
constexpr std::string_view KANJI_KAN = "\xb4\xc1"; //!< 漢
constexpr std::string_view KANJI_JI = "\xbb\xfa"; //!< 字
#endif

#ifdef JP
/*!
 * @brief 文字列を指定した回数だけ繰り返した文字列を作る
 * @param str 繰り返す文字列
 * @param count 繰り返す回数
 * @return 繰り返した文字列
 */
std::string repeat(std::string_view str, int count)
{
    std::string result;
    for (auto i = 0; i < count; ++i) {
        result.append(str);
    }

    return result;
}

//! 行末に2バイト文字の1バイト目だけが残った行
const std::string LINE_WITH_LONE_LEAD_BYTE = std::string("ab") + std::string(KANJI_KAN.substr(0, 1));
#endif

}

TEST_CASE("add_empty_line appends an empty line below the line limit")
{
    auto tb = make_text_body(MAX_EDITABLE_LINES - 1);

    CHECK(add_empty_line(&tb));
    CHECK(count_line(&tb) == MAX_EDITABLE_LINES);
    REQUIRE(tb.lines_list[MAX_EDITABLE_LINES - 1] != nullptr);
    CHECK(tb.lines_list[MAX_EDITABLE_LINES - 1]->empty());
    CHECK((tb.dirty_flags & DIRTY_EXPRESSION) != 0);
    CHECK(tb.changed);
}

TEST_CASE("add_empty_line does not append a line at the line limit")
{
    auto tb = make_text_body(MAX_EDITABLE_LINES);

    CHECK_FALSE(add_empty_line(&tb));
    CHECK(count_line(&tb) == MAX_EDITABLE_LINES);
    CHECK(tb.lines_list[MAX_EDITABLE_LINES] == nullptr);
    CHECK(tb.dirty_flags == 0);
    CHECK_FALSE(tb.changed);
}

TEST_CASE("add_empty_line does not append a line when the last line is empty")
{
    auto tb = make_text_body(1);
    *tb.lines_list[0] = "";

    CHECK_FALSE(add_empty_line(&tb));
    CHECK(count_line(&tb) == 1);
    CHECK_FALSE(tb.changed);
}

TEST_CASE("is_second_byte_of_kanji is false for single-byte characters")
{
    for (auto pos = 0; pos <= 3; pos++) {
        CAPTURE(pos);
        CHECK_FALSE(is_second_byte_of_kanji("abc", pos));
    }
}

TEST_CASE("insert_return_code splits the line at the cursor")
{
    auto tb = make_text_body_with_line("abcd", 2);

    REQUIRE(insert_return_code(&tb));
    CHECK(*tb.lines_list[0] == "ab");
    REQUIRE(tb.lines_list[1] != nullptr);
    CHECK(*tb.lines_list[1] == "cd");
}

#ifdef JP
TEST_CASE("is_second_byte_of_kanji detects the second byte of a double-byte character")
{
    const auto line = std::string(KANJI_KAN) + "a";

    CHECK_FALSE(is_second_byte_of_kanji(line, 0));
    CHECK(is_second_byte_of_kanji(line, 1));
    CHECK_FALSE(is_second_byte_of_kanji(line, 2));
}

TEST_CASE("is_second_byte_of_kanji does not treat the end of a line after a lone lead byte as the second byte")
{
    CHECK_FALSE(is_second_byte_of_kanji(LINE_WITH_LONE_LEAD_BYTE, 3));
}

TEST_CASE("insert_return_code does not split a double-byte character")
{
    auto tb = make_text_body_with_line(std::string(KANJI_KAN) + std::string(KANJI_JI), 1);

    REQUIRE(insert_return_code(&tb));
    CHECK(*tb.lines_list[0] == KANJI_KAN);
    REQUIRE(tb.lines_list[1] != nullptr);
    CHECK(*tb.lines_list[1] == KANJI_JI);
}

TEST_CASE("insert_return_code splits a line ending with a lone lead byte at its end")
{
    auto tb = make_text_body_with_line(LINE_WITH_LONE_LEAD_BYTE, 3);

    REQUIRE(insert_return_code(&tb));
    CHECK(*tb.lines_list[0] == LINE_WITH_LONE_LEAD_BYTE);
    REQUIRE(tb.lines_list[1] != nullptr);
    CHECK(tb.lines_list[1]->empty());
}

TEST_CASE("EC_LEFT moves back one double-byte character at a time")
{
    auto tb = make_text_body_with_line(std::string(KANJI_KAN) + std::string(KANJI_JI), 4);

    do_editor_command(nullptr, &tb, EC_LEFT);
    CHECK(tb.cx == 2);
    do_editor_command(nullptr, &tb, EC_LEFT);
    CHECK(tb.cx == 0);
}

TEST_CASE("insert_single_letter does not split a double-byte character pushed past the line limit")
{
    // 行の上限 (MAX_LINELEN - 1 バイト) ちょうどの行の先頭に1文字入れると、末尾の「漢」は丸ごと押し出される
    const auto line = "a" + repeat(KANJI_KAN, (MAX_LINELEN - 2) / 2);
    REQUIRE(line.length() == MAX_LINELEN - 1);
    auto tb = make_text_body_with_line(line, 0);

    insert_single_letter(&tb, 'x');
    CHECK(*tb.lines_list[0] == "xa" + repeat(KANJI_KAN, (MAX_LINELEN - 2) / 2 - 1));
}

TEST_CASE("EC_PASTE does not split a double-byte character pushed past the line limit")
{
    const auto line = "a" + repeat(KANJI_KAN, (MAX_LINELEN - 4) / 2);
    REQUIRE(line.length() == MAX_LINELEN - 3);
    auto tb = make_text_body_with_line(line, 0);
    tb.yank = { "xyz" };

    do_editor_command(nullptr, &tb, EC_PASTE);
    CHECK(*tb.lines_list[0] == "xyza" + repeat(KANJI_KAN, (MAX_LINELEN - 4) / 2 - 1));
    CHECK(tb.cx == 3);
}

TEST_CASE("EC_LEFT moves back one byte over a lone lead byte at the end of a line")
{
    auto tb = make_text_body_with_line(LINE_WITH_LONE_LEAD_BYTE, 3);

    do_editor_command(nullptr, &tb, EC_LEFT);
    CHECK(tb.cx == 2);
}
#endif
