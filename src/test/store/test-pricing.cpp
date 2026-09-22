/*!
 * @brief 店舗の価格計算のテスト
 *
 * 店主の強欲さと補正の和 (markup) から店舗価格を計算する calc_store_price() について、
 * 売出価格と買取価格の補正の範囲、丸め、安い品物の扱い、闇市の倍率を検証する。
 */

#include "store/pricing.h"

#include <doctest/doctest.h>

namespace {
constexpr auto NEUTRAL_MARKUP = 300; //!< 補正が 100% になる markup
constexpr auto PURCHASE = false; //!< プレイヤーが店から買う (店の売出価格)
constexpr auto SALE = true; //!< プレイヤーが店に売る (店の買取価格)
}

TEST_CASE("calc_store_price returns 0 for items without value")
{
    CHECK(calc_store_price(0, NEUTRAL_MARKUP, tl::nullopt, PURCHASE) == 0);
    CHECK(calc_store_price(-1, NEUTRAL_MARKUP, tl::nullopt, SALE) == 0);
    CHECK(calc_store_price(0, NEUTRAL_MARKUP, 1, PURCHASE) == 0);
}

TEST_CASE("calc_store_price never lets the shopkeeper lose money")
{
    // 買うときの補正は 100% 未満にならず、売るときの補正は 100% を超えない
    CHECK(calc_store_price(1000, 250, tl::nullopt, PURCHASE) == 1100);
    CHECK(calc_store_price(1000, 250, tl::nullopt, SALE) == 900);

    CHECK(calc_store_price(1000, 320, tl::nullopt, PURCHASE) == 1320);
    CHECK(calc_store_price(1000, 320, tl::nullopt, SALE) == 720);
}

TEST_CASE("calc_store_price rounds the adjusted price")
{
    CHECK(calc_store_price(50, 301, tl::nullopt, PURCHASE) == 56);
    CHECK(calc_store_price(49, 301, tl::nullopt, PURCHASE) == 53);
}

TEST_CASE("calc_store_price adds no margin to cheap items")
{
    CHECK(calc_store_price(9, NEUTRAL_MARKUP, tl::nullopt, PURCHASE) == 9);
    CHECK(calc_store_price(9, NEUTRAL_MARKUP, tl::nullopt, SALE) == 9);
    CHECK(calc_store_price(10, NEUTRAL_MARKUP, tl::nullopt, PURCHASE) == 11);
    CHECK(calc_store_price(10, NEUTRAL_MARKUP, tl::nullopt, SALE) == 9);
}

TEST_CASE("calc_store_price in the black market")
{
    SUBCASE("halves the price when the player sells")
    {
        CHECK(calc_store_price(1000, NEUTRAL_MARKUP, 50, SALE) == 450);
        CHECK(calc_store_price(1, NEUTRAL_MARKUP, 50, SALE) == 1);
    }

    SUBCASE("multiplies the price by the level when the player buys")
    {
        CHECK(calc_store_price(1000, NEUTRAL_MARKUP, 1, PURCHASE) == 2200);
        CHECK(calc_store_price(1000, NEUTRAL_MARKUP, 2, PURCHASE) == 2222);
    }
}
