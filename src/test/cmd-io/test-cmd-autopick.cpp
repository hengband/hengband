/*!
 * @brief 自動拾いエディタのカーソル列補正のテスト
 */

#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-editor-util.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"

#include <doctest/doctest.h>

#include <initializer_list>
#include <string>
#include <vector>

namespace {

//! 先頭からlines.size() の行数だけ文字列を埋めたAutopickTextBody を作る.
AutopickTextBody make_text_body(std::initializer_list<std::string> lines)
{
    AutopickTextBody tb(0, 0);
    tb.set_lines(lines);

    return tb;
}

}

TEST_CASE("adjust_cursor_column clamps the mark column by the line the mark is on")
{
    auto tb = make_text_body({ "aaaaaaaa", "aa" });
    tb.cx = 8;
    tb.marks.set(AutopickMark::MARK);
    tb.my = 1;
    tb.mx = 5;

    tb.adjust_cursor_column();

    CHECK(tb.mx == 2);
    CHECK(tb.cx == 8);
}

TEST_CASE("adjust_cursor_column does not touch the mark column when nothing is marked")
{
    auto tb = make_text_body({ "aaaa" });
    tb.my = 5;
    tb.mx = 10;

    tb.adjust_cursor_column();

    CHECK(tb.mx == 10);
}

TEST_CASE("adjust_cursor_column keeps the mark column within the line shortened by toggling the command letter")
{
    // 行末にマークを置いたまま、破壊の指定を外して行を短くする
    auto tb = make_text_body({ "!aaaa" });
    tb.cx = 5;
    tb.marks.set(AutopickMark::MARK);
    tb.mx = 5;
    toggle_command_letter(&tb, AutopickMethod::AUTOPICK);
    REQUIRE(tb.lines[0].line == "aaaa");

    tb.adjust_cursor_column();

    CHECK(tb.mx == 4);
    CHECK(tb.cx == 4);
}

TEST_CASE("adjust_cursor_column puts the cursor at the recorded column within the line")
{
    auto tb = make_text_body({ "aaaaaa" });

    tb.rec_cx = 3;
    tb.adjust_cursor_column();
    CHECK(tb.cx == 3);

    tb.rec_cx = 10;
    tb.adjust_cursor_column();
    CHECK(tb.cx == 6);
}

TEST_CASE("update_cursor_column_record keeps the widest column while moving up and down")
{
    auto tb = make_text_body({ "aaaaaa" });

    tb.update_cursor_column_record(EditorCommandId::UP, 0, 4);
    CHECK(tb.rec_cx == 4);
    tb.update_cursor_column_record(EditorCommandId::DOWN, 0, 2);
    CHECK(tb.rec_cx == 4);
    tb.update_cursor_column_record(EditorCommandId::LEFT, 0, 4);
    CHECK(tb.rec_cx == 0);
}

TEST_CASE("update_cursor_column_record records the column after the command when the command moves the cursor")
{
    auto tb = make_text_body({ "aaaaaa" });

    // BOTTOM はカーソルを行頭へ移すので、移す前の桁は覚えない
    tb.cx = 0;
    tb.update_cursor_column_record(EditorCommandId::BOTTOM, 4, 4);
    CHECK(tb.rec_cx == 0);
}

TEST_CASE("set_lines replaces all the lines")
{
    auto tb = make_text_body({ "aa", "bb", "cc" });

    tb.set_lines({ "dd" });
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "dd" });
}

TEST_CASE("set_lines keeps an empty line when no lines are given")
{
    auto tb = make_text_body({ "aa", "bb" });

    tb.set_lines({});
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "" });
}
