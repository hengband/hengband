/*!
 * @brief 店舗の画面 (StoreScreen) のテスト
 *
 * 端末の高さから決まるページの大きさと行の位置、ページ送り・ページ戻し、
 * 在庫の増減に合わせたページの調整を検証する。
 */

#include "store/store-screen.h"

#include "game-option/birth-options.h"
#include "store/store-util.h"
#include "term/gameterm.h"
#include "util/finalizer.h"

#include <doctest/doctest.h>

namespace {
constexpr auto DEFAULT_HEIGHT = MAIN_TERM_MIN_ROWS; //!< ページの大きさが最小 (12) になる端末の高さ
}

TEST_CASE("StoreScreen derives page size and rows from the terminal height")
{
    Store store(StoreSaleType::GENERAL);

    const StoreScreen low(store, 0, DEFAULT_HEIGHT - 1);
    CHECK(low.get_page_size() == 12);
    CHECK(low.get_status_row() == 19);
    CHECK(low.get_command_row(0) == 20);

    const StoreScreen normal(store, 0, DEFAULT_HEIGHT);
    CHECK(normal.get_page_size() == 12);
    CHECK(normal.get_status_row() == 19);
    CHECK(normal.get_command_row(3) == 23);

    const StoreScreen tall(store, 0, DEFAULT_HEIGHT + 6);
    CHECK(tall.get_page_size() == 18);
    CHECK(tall.get_status_row() == 25);
    CHECK(tall.get_command_row(1) == 27);

    const StoreScreen highest(store, 0, DEFAULT_HEIGHT + 40);
    CHECK(highest.get_page_size() == 52);
    const StoreScreen too_high(store, 0, DEFAULT_HEIGHT + 100);
    CHECK(too_high.get_page_size() == 52);
}

TEST_CASE("StoreScreen starts on the first page and counts the items on it")
{
    Store store(StoreSaleType::GENERAL);
    StoreScreen screen(store, 0, DEFAULT_HEIGHT);
    CHECK(screen.get_page_top() == 0);

    store.stock_num = 0;
    CHECK(screen.get_page_item_count() == 0);
    CHECK_FALSE(screen.has_multiple_pages());

    store.stock_num = 12;
    CHECK(screen.get_page_item_count() == 12);
    CHECK_FALSE(screen.has_multiple_pages());

    store.stock_num = 30;
    CHECK(screen.get_page_item_count() == 12);
    CHECK(screen.has_multiple_pages());
}

TEST_CASE("StoreScreen::turn_page_forward wraps to the first page")
{
    const auto restore = util::make_finalizer([old = powerup_home] { powerup_home = old; });
    powerup_home = false;

    SUBCASE("at the end of the stock")
    {
        Store store(StoreSaleType::GENERAL);
        StoreScreen screen(store, 0, DEFAULT_HEIGHT);
        store.stock_num = 30;
        screen.turn_page_forward();
        CHECK(screen.get_page_top() == 12);
        CHECK(screen.get_page_item_count() == 12);
        screen.turn_page_forward();
        CHECK(screen.get_page_top() == 24);
        CHECK(screen.get_page_item_count() == 6);
        screen.turn_page_forward();
        CHECK(screen.get_page_top() == 0);
    }

    SUBCASE("at the second page of a home without powerup_home")
    {
        Store store(StoreSaleType::HOME);
        StoreScreen screen(store, 0, DEFAULT_HEIGHT);
        store.stock_num = 30;
        screen.turn_page_forward();
        CHECK(screen.get_page_top() == 12);
        screen.turn_page_forward();
        CHECK(screen.get_page_top() == 0);
    }
}

TEST_CASE("StoreScreen::turn_page_backward wraps to the last page")
{
    const auto restore = util::make_finalizer([old = powerup_home] { powerup_home = old; });
    powerup_home = false;

    SUBCASE("in a store")
    {
        Store store(StoreSaleType::GENERAL);
        StoreScreen screen(store, 0, DEFAULT_HEIGHT);
        store.stock_num = 30;
        screen.turn_page_backward();
        CHECK(screen.get_page_top() == 24);
        screen.turn_page_backward();
        CHECK(screen.get_page_top() == 12);
        screen.turn_page_backward();
        CHECK(screen.get_page_top() == 0);
    }

    SUBCASE("in a home without powerup_home, which shows only two pages")
    {
        Store store(StoreSaleType::HOME);
        StoreScreen screen(store, 0, DEFAULT_HEIGHT);
        store.stock_num = 30;
        screen.turn_page_backward();
        CHECK(screen.get_page_top() == 12);
    }

    SUBCASE("in a home with powerup_home")
    {
        powerup_home = true;
        Store store(StoreSaleType::HOME);
        StoreScreen screen(store, 0, DEFAULT_HEIGHT);
        store.stock_num = 30;
        screen.turn_page_backward();
        CHECK(screen.get_page_top() == 24);
    }

    SUBCASE("when the stock ends at a page boundary")
    {
        Store store(StoreSaleType::GENERAL);
        StoreScreen screen(store, 0, DEFAULT_HEIGHT);
        store.stock_num = 24;
        screen.turn_page_backward();
        CHECK(screen.get_page_top() == 12);
    }
}

TEST_CASE("StoreScreen::show_page_containing shows the page of the item")
{
    Store store(StoreSaleType::GENERAL);
    StoreScreen screen(store, 0, DEFAULT_HEIGHT);
    store.stock_num = 30;

    for (const auto &[pos, page_top] : { std::pair{ 0, 0 }, { 11, 0 }, { 12, 12 }, { 29, 24 } }) {
        CAPTURE(pos);
        screen.show_page_containing(pos);
        CHECK(screen.get_page_top() == page_top);
    }
}

TEST_CASE("StoreScreen::adjust_page_after_removal goes back when the page becomes empty")
{
    Store store(StoreSaleType::GENERAL);
    StoreScreen screen(store, 0, DEFAULT_HEIGHT);

    SUBCASE("the page still has items")
    {
        store.stock_num = 13;
        screen.show_page_containing(12);
        screen.adjust_page_after_removal();
        CHECK(screen.get_page_top() == 12);
    }

    SUBCASE("the last page becomes empty")
    {
        store.stock_num = 13;
        screen.show_page_containing(12);
        store.stock_num = 12;
        screen.adjust_page_after_removal();
        CHECK(screen.get_page_top() == 0);
    }

    SUBCASE("the stock becomes empty")
    {
        store.stock_num = 0;
        screen.adjust_page_after_removal();
        CHECK(screen.get_page_top() == 0);
    }
}

TEST_CASE("StoreScreen::get_page_position matches the row index on every page")
{
    Store store(StoreSaleType::GENERAL);
    StoreScreen screen(store, 0, DEFAULT_HEIGHT + 3);
    store.stock_num = 40;

    for (auto page = 0; page < 3; ++page) {
        CAPTURE(page);
        for (auto pos = screen.get_page_top(); pos < screen.get_page_top() + screen.get_page_item_count(); ++pos) {
            CAPTURE(pos);
            CHECK(screen.get_page_position(pos) == pos % screen.get_page_size());
        }

        screen.turn_page_forward();
    }
}
