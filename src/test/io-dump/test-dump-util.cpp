/*!
 * @brief 知識の一覧のカーソル移動と、シンボルの一覧の左端の文字のテスト
 */

#include "io-dump/dump-util.h"
#include "test/scoped-input-term.h"
#include "test/scoped-restore.h"
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

TEST_CASE("calc_visual_list_char_left starts the list 10 characters before the selected one")
{
    CHECK(calc_visual_list_char_left('.') == 0x24);
    CHECK(calc_visual_list_char_left('\x05') == 0);
    CHECK(calc_visual_list_char_left('\x0a') == 0);
    CHECK(calc_visual_list_char_left('\x7f') == 0x75);
}

TEST_CASE("calc_visual_list_char_left treats graphic tiles as unsigned characters")
{
    // 0x80 以上の文字は char が符号付きの環境では負になるが、符号なしの文字として 10 文字手前を返す
    CHECK(calc_visual_list_char_left('\x80') == 0x76);
    CHECK(calc_visual_list_char_left('\x89') == 0x7f);
    CHECK(calc_visual_list_char_left('\xa2') == 0x98);
    CHECK(calc_visual_list_char_left('\xff') == 0xf5);
}

TEST_CASE("visual_mode_command starts the visual list near a graphic tile")
{
    auto &clipboard = DisplaySymbolsClipboard::get_instance();
    const auto restore_clipboard = test::scoped_restore(clipboard.symbol, clipboard.symbols);
    auto visual_list = false;
    TERM_COLOR attr_top = 0;
    byte char_left = 0;
    TERM_COLOR attr = 0x85;
    auto character = '\xa2';
    auto need_redraw = false;

    SUBCASE("v opens the list around the current tile")
    {
        CHECK(visual_mode_command('v', &visual_list, 15, 69, &attr_top, &char_left, &attr, &character, &need_redraw));
        CHECK(visual_list);
        CHECK(char_left == 0x98);
    }

    SUBCASE("p moves the list to the pasted tile")
    {
        visual_list = true;
        clipboard.symbol = { 0x86, '\xc8' };
        CHECK(visual_mode_command('p', &visual_list, 15, 69, &attr_top, &char_left, &attr, &character, &need_redraw));
        CHECK(static_cast<uint8_t>(character) == 0xc8);
        CHECK(char_left == 0xbe);
    }
}
