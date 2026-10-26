/*!
 * @brief 2 列のメニューでカーソルを反対の列に移す処理 (menu-column-switcher.h) のテスト
 *
 * 反対の列の行に移ること、空の行から項目のある行を探す順、一覧の端の行から探しても一覧の外を調べないことを検証する。
 */

#include "util/menu-column-switcher.h"

#include <doctest/doctest.h>
#include <set>
#include <utility>
#include <vector>

namespace {
/*!
 * @brief 項目のある行 (0 から数える) を受け、調べた行を記録しながら項目の有無を返す関数を作る
 */
auto make_is_filled(std::set<int> filled, std::vector<int> &visited)
{
    return [filled = std::move(filled), &visited](int i) {
        visited.push_back(i);
        return filled.contains(i);
    };
}

/*!
 * @brief from から to までの行 (両端を含む) を、1 ずつ増やすか減らして並べる
 * @param head 先頭に置く行
 */
std::vector<int> make_lines(std::vector<int> head, int from, int to)
{
    const auto step = from <= to ? 1 : -1;
    for (auto i = from; i != to + step; i += step) {
        head.push_back(i);
    }

    return head;
}
}

TEST_CASE("switch_menu_column moves to the same height of the other column")
{
    const auto all_filled = [](int) { return true; };
    CHECK(switch_menu_column(2, false, 8, all_filled) == 6);
    CHECK(switch_menu_column(6, false, 8, all_filled) == 2);
    CHECK(switch_menu_column(2, true, 8, all_filled) == 6);
    CHECK(switch_menu_column(6, true, 8, all_filled) == 2);
}

TEST_CASE("switch_menu_column searches for a filled line from an empty line")
{
    std::vector<int> visited;

    SUBCASE("downward from the left column")
    {
        CHECK(switch_menu_column(1, false, 8, make_is_filled({ 0, 6 }, visited)) == 7);
        CHECK(visited == std::vector<int>{ 4, 5, 6 });
    }

    SUBCASE("upward from the left column with a key to the left")
    {
        CHECK(switch_menu_column(3, true, 8, make_is_filled({ 2, 4 }, visited)) == 5);
        CHECK(visited == std::vector<int>{ 6, 5, 4 });
    }

    SUBCASE("upward from the right column regardless of the key")
    {
        for (const auto is_left_key : { false, true }) {
            visited.clear();
            CHECK(switch_menu_column(8, is_left_key, 8, make_is_filled({ 1, 7 }, visited)) == 2);
            CHECK(visited == std::vector<int>{ 3, 2, 1 });
        }
    }
}

TEST_CASE("switch_menu_column turns back at the ends of the list without looking outside")
{
    std::vector<int> visited;

    SUBCASE("from the last line of the left column in a list of even size")
    {
        // マジック・ミサイルの魔法棒だけを取り込み、16 行目から右へ移る場合 (#2956)
        CHECK(switch_menu_column(16, false, 32, make_is_filled({ 15 }, visited)) == 16);
        CHECK(visited == make_lines({}, 31, 15));
    }

    SUBCASE("from the first line of the right column")
    {
        CHECK(switch_menu_column(17, false, 32, make_is_filled({ 16 }, visited)) == 17);
        CHECK(visited == make_lines({}, 0, 16));
    }

    SUBCASE("in a list of odd size, whose right column is one line longer")
    {
        // 移った先の行は、次の行 (一覧の端) で向きを変えた後にもう一度調べる (変更前と同じ順)
        CHECK(switch_menu_column(15, false, 31, make_is_filled({ 14 }, visited)) == 15);
        CHECK(visited == make_lines({ 29, 30 }, 29, 14));
        visited.clear();
        CHECK(switch_menu_column(16, false, 33, make_is_filled({ 15 }, visited)) == 16);
        CHECK(visited == make_lines({ 31, 32 }, 31, 15));
    }
}
