/*!
 * @brief 自動拾いエディタのカーソル列補正のテスト
 */

#include "autopick/autopick-editor-util.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"

#include <doctest/doctest.h>

#include <initializer_list>
#include <memory>
#include <string>

namespace {

//! 指定の行を持つエディタの状態を、実際のエディタと同じく MAX_LINES 要素の行バッファで作る
text_body_type make_text_body(std::initializer_list<std::string> lines)
{
    text_body_type tb(0, 0);
    tb.lines_list.resize(MAX_LINES);
    auto y = 0;
    for (const auto &line : lines) {
        tb.lines_list[y++] = std::make_unique<std::string>(line);
    }

    return tb;
}

}

TEST_CASE("adjust_cursor_column clamps the mark column by the line the mark is on")
{
    auto tb = make_text_body({ "aaaaaaaa", "aa" });
    tb.cx = 8;
    tb.mark = MARK_MARK;
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
    tb.mark = MARK_MARK;
    tb.mx = 5;
    toggle_command_letter(&tb, AutopickMethod::AUTOPICK);
    REQUIRE(*tb.lines_list[0] == "aaaa");

    tb.adjust_cursor_column();

    CHECK(tb.mx == 4);
    CHECK(tb.cx == 4);
}
