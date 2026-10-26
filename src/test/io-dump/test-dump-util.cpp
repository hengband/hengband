/*!
 * @brief 知識の一覧のカーソル移動のテスト
 */

#include "io-dump/dump-util.h"
#include "test/scoped-input-term.h"
#include <doctest/doctest.h>

TEST_CASE("browser_cursor pages within a list near the IDX maximum")
{
    // 80x24 の端末では、一覧の1ページは 24 - 8 = 16 行になる。
    test::ScopedInputTerm term;
    constexpr auto count = 32767;

    SUBCASE("page down in the list column stops at the last entry")
    {
        auto column = 1;
        IDX grp = 0;
        IDX list = 32760;
        browser_cursor(' ', &column, &grp, 1, &list, count);
        CHECK(list == 32766);
        CHECK(grp == 0);
    }

    SUBCASE("page down in the group column stops at the last group")
    {
        auto column = 0;
        IDX grp = 32760;
        IDX list = 5;
        browser_cursor(' ', &column, &grp, count, &list, 1);
        CHECK(grp == 32766);
        CHECK(list == 0);
    }

    SUBCASE("page up near the top stops at the first entry")
    {
        auto column = 1;
        IDX grp = 0;
        IDX list = 5;
        browser_cursor('-', &column, &grp, 1, &list, count);
        CHECK(list == 0);
    }
}
