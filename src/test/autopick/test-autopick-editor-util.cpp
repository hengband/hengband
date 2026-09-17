/*!
 * @brief 自動拾いエディタの編集操作のテスト
 */

#include "autopick/autopick-editor-util.h"

#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-util.h"

#include <doctest/doctest.h>

#include <memory>
#include <string>

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
