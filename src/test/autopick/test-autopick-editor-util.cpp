/*!
 * @brief 自動拾いエディタの編集操作のテスト
 */

#include "autopick/autopick-editor-util.h"

#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-editor-command.h"
#include "autopick/autopick-inserter-killer.h"
#include "autopick/autopick-util.h"
#include "system/item/item-entity.h"

#include "test/scoped-restore.h"
#include "test/string-helpers.h"

#include <doctest/doctest.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace test;

namespace {

//! is_greater_autopick_max_line() が偽になる最大の行数
constexpr auto MAX_EDITABLE_LINES = MAX_AUTOPICK_ENTRIES - 2;

/*!
 * @brief 空でない行を指定の行数だけ持つエディタの状態を作る
 * @param num_lines 行数
 * @return エディタの状態
 */
AutopickTextBody make_text_body(int num_lines)
{
    AutopickTextBody tb(0, 0);
    tb.set_lines(std::vector<std::string>(num_lines, "a"));

    return tb;
}

/*!
 * @brief 1行だけを持ち、カーソルがその行にあるエディタの状態を作る
 * @param line 行の内容
 * @param cx カーソルの位置 (バイト)
 * @return エディタの状態
 */
AutopickTextBody make_text_body_with_line(std::string_view line, int cx)
{
    auto tb = make_text_body(1);
    *tb.lines[0].line = line;
    tb.cx = cx;
    return tb;
}

#ifdef JP
//! 行末に2バイト文字の1バイト目だけが残った行
const std::string LINE_WITH_LONE_LEAD_BYTE = cat("ab", KANJI_KAN.substr(0, 1));
#endif

}

TEST_CASE("add_empty_line appends an empty line below the line limit")
{
    auto tb = make_text_body(MAX_EDITABLE_LINES - 1);

    CHECK(add_empty_line(&tb));
    REQUIRE(tb.lines.size() == MAX_EDITABLE_LINES);
    CHECK(tb.lines[MAX_EDITABLE_LINES - 1].line->empty());
    CHECK((tb.dirty_flags & DIRTY_EXPRESSION) != 0);
    CHECK(tb.changed);
}

TEST_CASE("add_empty_line does not append a line at the line limit")
{
    auto tb = make_text_body(MAX_EDITABLE_LINES);

    CHECK_FALSE(add_empty_line(&tb));
    CHECK(tb.lines.size() == MAX_EDITABLE_LINES);
    CHECK(tb.dirty_flags == 0);
    CHECK_FALSE(tb.changed);
}

TEST_CASE("add_empty_line does not append a line when the last line is empty")
{
    auto tb = make_text_body(1);
    *tb.lines[0].line = "";

    CHECK_FALSE(add_empty_line(&tb));
    CHECK(tb.lines.size() == 1);
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
    CHECK(*tb.lines[0].line == "ab");
    REQUIRE(tb.lines.size() == 2);
    CHECK(*tb.lines[1].line == "cd");
}

TEST_CASE("insert_return_code shifts the following lines down")
{
    AutopickTextBody tb(1, 1);
    tb.set_lines({ "ab", "cd", "ef" });

    REQUIRE(insert_return_code(&tb));
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "ab", "c", "d", "ef" });
}

TEST_CASE("EditorCommandId::BACKSPACE at the beginning of a line joins it to the previous line")
{
    AutopickTextBody tb(0, 1);
    tb.set_lines({ "ab", "cd", "ef" });

    do_editor_command(nullptr, &tb, EditorCommandId::BACKSPACE);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "abcd", "ef" });
    CHECK(tb.cx == 2);
    CHECK(tb.cy == 0);
}

TEST_CASE("kill_line_segment removes a whole line and shifts the following lines up")
{
    AutopickTextBody tb(0, 0);
    tb.set_lines({ "ab", "cd", "ef" });

    kill_line_segment(&tb, 1, 0, 2, true);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "ab", "ef" });
}

TEST_CASE("kill_line_segment empties the last line instead of removing it")
{
    AutopickTextBody tb(0, 0);
    tb.set_lines({ "ab", "cd" });

    kill_line_segment(&tb, 1, 0, 2, true);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "ab", "" });
}

TEST_CASE("EditorCommandId::DOWN on the last line appends an empty line")
{
    AutopickTextBody tb(0, 0);
    tb.set_lines({ "a" });

    do_editor_command(nullptr, &tb, EditorCommandId::DOWN);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "a", "" });
    CHECK(tb.cy == 1);
}

TEST_CASE("EditorCommandId::DOWN on an empty last line stays there")
{
    AutopickTextBody tb(0, 1);
    tb.set_lines({ "a", "" });

    do_editor_command(nullptr, &tb, EditorCommandId::DOWN);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "a", "" });
    CHECK(tb.cy == 1);
}

TEST_CASE("EditorCommandId::BOTTOM appends an empty line after a non-empty last line and moves there")
{
    AutopickTextBody tb(1, 0);
    tb.set_lines({ "a", "b" });

    do_editor_command(nullptr, &tb, EditorCommandId::BOTTOM);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "a", "b", "" });
    CHECK(tb.cy == 2);
    CHECK(tb.cx == 0);
}

TEST_CASE("EditorCommandId::BOTTOM moves to an empty last line without appending a line")
{
    AutopickTextBody tb(1, 0);
    tb.set_lines({ "a", "" });

    do_editor_command(nullptr, &tb, EditorCommandId::BOTTOM);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "a", "" });
    CHECK(tb.cy == 1);
    CHECK(tb.cx == 0);
}

TEST_CASE("EditorCommandId::CUT of lines up to the last line removes them and empties the last line")
{
    AutopickTextBody tb(1, 2);
    tb.set_lines({ "a", "b", "c" });
    tb.marks.set(AutopickMark::MARK);
    tb.my = 1;
    tb.mx = 0;

    do_editor_command(nullptr, &tb, EditorCommandId::CUT);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "a", "" });
    CHECK(tb.cy == 1);
    CHECK(tb.cx == 0);
}

TEST_CASE("EditorCommandId::PASTE of whole lines inserts them before the rest of the line")
{
    AutopickTextBody tb(1, 0);
    tb.set_lines({ "ab" });
    tb.yank = { "x", "y" };
    tb.yank_eol = true;

    do_editor_command(nullptr, &tb, EditorCommandId::PASTE);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "ax", "y", "b" });
    CHECK(tb.cy == 2);
    CHECK(tb.cx == 0);
}

TEST_CASE("auto_inscribe_item returns false for invalid index or empty insc")
{
    const auto restore = test::scoped_restore(autopick_list);
    autopick_list.clear();
    autopick_list.emplace_back(); // insc空のエントリを用意

    ItemEntity item;
    // idx < 0
    CHECK_FALSE(auto_inscribe_item(item, -1));
    CHECK_FALSE(item.is_inscribed());

    // insc空
    CHECK_FALSE(auto_inscribe_item(item, 0));
    CHECK_FALSE(item.is_inscribed());
}

TEST_CASE("auto_inscribe_item does not overwrite existing inscription and returns false")
{
    const auto restore = test::scoped_restore(autopick_list);
    autopick_list.clear();
    autopick_type entry;
    entry.insc = "test-insc";
    autopick_list.push_back(entry);

    ItemEntity item;
    item.inscription = "existing";
    REQUIRE(item.is_inscribed());

    CHECK_FALSE(auto_inscribe_item(item, 0));
    CHECK(item.inscription == "existing"); // 上書きされない
}

TEST_CASE("auto_inscribe_item sets inscription when empty and returns true")
{
    const auto restore = test::scoped_restore(autopick_list);
    autopick_list.clear();
    autopick_type entry;
    entry.insc = "new-insc";
    autopick_list.push_back(entry);

    ItemEntity item;
    REQUIRE_FALSE(item.is_inscribed());

    CHECK(auto_inscribe_item(item, 0));
    CHECK(item.inscription == "new-insc");
}

#ifdef JP
TEST_CASE("is_second_byte_of_kanji detects the second byte of a double-byte character")
{
    const auto line = cat(KANJI_KAN, "a");

    CHECK_FALSE(is_second_byte_of_kanji(line, 0));
    CHECK(is_second_byte_of_kanji(line, 1));
    CHECK_FALSE(is_second_byte_of_kanji(line, 2));
}

TEST_CASE("is_second_byte_of_kanji does not treat the end of a line after a lone lead byte as the second byte")
{
    CHECK_FALSE(is_second_byte_of_kanji(LINE_WITH_LONE_LEAD_BYTE, 3));
}

TEST_CASE("is_cursor_on_second_byte_of_kanji checks the cursor position on the cursor line")
{
    const auto line = cat(KANJI_KAN, "a");

    CHECK_FALSE(make_text_body_with_line(line, 0).is_cursor_on_second_byte_of_kanji());
    CHECK(make_text_body_with_line(line, 1).is_cursor_on_second_byte_of_kanji());
    CHECK_FALSE(make_text_body_with_line(line, 2).is_cursor_on_second_byte_of_kanji());
    CHECK_FALSE(make_text_body_with_line(line, 10).is_cursor_on_second_byte_of_kanji());
}

TEST_CASE("insert_return_code does not split a double-byte character")
{
    auto tb = make_text_body_with_line(cat(KANJI_KAN, KANJI_JI), 1);

    REQUIRE(insert_return_code(&tb));
    CHECK(*tb.lines[0].line == KANJI_KAN);
    REQUIRE(tb.lines.size() == 2);
    CHECK(*tb.lines[1].line == KANJI_JI);
}

TEST_CASE("insert_return_code splits a line ending with a lone lead byte at its end")
{
    auto tb = make_text_body_with_line(LINE_WITH_LONE_LEAD_BYTE, 3);

    REQUIRE(insert_return_code(&tb));
    CHECK(*tb.lines[0].line == LINE_WITH_LONE_LEAD_BYTE);
    REQUIRE(tb.lines.size() == 2);
    CHECK(tb.lines[1].line->empty());
}

TEST_CASE("EditorCommandId::LEFT moves back one double-byte character at a time")
{
    auto tb = make_text_body_with_line(cat(KANJI_KAN, KANJI_JI), 4);

    do_editor_command(nullptr, &tb, EditorCommandId::LEFT);
    CHECK(tb.cx == 2);
    do_editor_command(nullptr, &tb, EditorCommandId::LEFT);
    CHECK(tb.cx == 0);
}

TEST_CASE("insert_single_letter does not split a double-byte character pushed past the line limit")
{
    // 行の上限 (MAX_LINELEN - 1 バイト) ちょうどの行の先頭に1文字入れると、末尾の「漢」は丸ごと押し出される
    const auto line = "a" + repeat(KANJI_KAN, (MAX_LINELEN - 2) / 2);
    REQUIRE(line.length() == MAX_LINELEN - 1);
    auto tb = make_text_body_with_line(line, 0);

    insert_single_letter(&tb, 'x');
    CHECK(*tb.lines[0].line == "xa" + repeat(KANJI_KAN, (MAX_LINELEN - 2) / 2 - 1));
}

TEST_CASE("EditorCommandId::PASTE does not split a double-byte character pushed past the line limit")
{
    const auto line = "a" + repeat(KANJI_KAN, (MAX_LINELEN - 4) / 2);
    REQUIRE(line.length() == MAX_LINELEN - 3);
    auto tb = make_text_body_with_line(line, 0);
    tb.yank = { "xyz" };

    do_editor_command(nullptr, &tb, EditorCommandId::PASTE);
    CHECK(*tb.lines[0].line == "xyza" + repeat(KANJI_KAN, (MAX_LINELEN - 4) / 2 - 1));
    CHECK(tb.cx == 3);
}

TEST_CASE("EditorCommandId::LEFT moves back one byte over a lone lead byte at the end of a line")
{
    auto tb = make_text_body_with_line(LINE_WITH_LONE_LEAD_BYTE, 3);

    do_editor_command(nullptr, &tb, EditorCommandId::LEFT);
    CHECK(tb.cx == 2);
}
#endif
