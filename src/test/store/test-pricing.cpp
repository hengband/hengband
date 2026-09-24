/*!
 * @brief 店舗の価格計算のテスト
 *
 * 店主の強欲さと補正の和 (markup) から店舗価格を計算する calc_store_price() について、
 * 売出価格と買取価格の補正の範囲、丸め、安い品物の扱い、闇市の倍率を検証する。
 * 闇市の倍率の表 (get_black_market_multiplier()) が、レベルごとの漸化式と一致することも検証する。
 */

#include "store/pricing.h"

#include <doctest/doctest.h>

namespace {
constexpr auto NEUTRAL_MARKUP = 300; //!< 補正が 100% になる markup
constexpr auto PLAYER_BUYS = StoreTradeType::PLAYER_BUYS;
constexpr auto PLAYER_SELLS = StoreTradeType::PLAYER_SELLS;
}

TEST_CASE("calc_store_price returns 0 for items without value")
{
    CHECK(calc_store_price(0, NEUTRAL_MARKUP, tl::nullopt, PLAYER_BUYS) == 0);
    CHECK(calc_store_price(-1, NEUTRAL_MARKUP, tl::nullopt, PLAYER_SELLS) == 0);
    CHECK(calc_store_price(0, NEUTRAL_MARKUP, 1, PLAYER_BUYS) == 0);
}

TEST_CASE("calc_store_price never lets the shopkeeper lose money")
{
    // 買うときの補正は 100% 未満にならず、売るときの補正は 100% を超えない
    CHECK(calc_store_price(1000, 250, tl::nullopt, PLAYER_BUYS) == 1100);
    CHECK(calc_store_price(1000, 250, tl::nullopt, PLAYER_SELLS) == 900);

    CHECK(calc_store_price(1000, 320, tl::nullopt, PLAYER_BUYS) == 1320);
    CHECK(calc_store_price(1000, 320, tl::nullopt, PLAYER_SELLS) == 720);
}

TEST_CASE("calc_store_price rounds the adjusted price")
{
    CHECK(calc_store_price(50, 301, tl::nullopt, PLAYER_BUYS) == 56);
    CHECK(calc_store_price(49, 301, tl::nullopt, PLAYER_BUYS) == 53);
}

TEST_CASE("calc_store_price adds no margin to cheap items")
{
    CHECK(calc_store_price(9, NEUTRAL_MARKUP, tl::nullopt, PLAYER_BUYS) == 9);
    CHECK(calc_store_price(9, NEUTRAL_MARKUP, tl::nullopt, PLAYER_SELLS) == 9);
    CHECK(calc_store_price(10, NEUTRAL_MARKUP, tl::nullopt, PLAYER_BUYS) == 11);
    CHECK(calc_store_price(10, NEUTRAL_MARKUP, tl::nullopt, PLAYER_SELLS) == 9);
}

TEST_CASE("calc_store_price in the black market")
{
    SUBCASE("halves the price when the player sells")
    {
        CHECK(calc_store_price(1000, NEUTRAL_MARKUP, 50, PLAYER_SELLS) == 450);
        CHECK(calc_store_price(1, NEUTRAL_MARKUP, 50, PLAYER_SELLS) == 1);
    }

    SUBCASE("multiplies the price by the level when the player buys")
    {
        CHECK(calc_store_price(1000, NEUTRAL_MARKUP, 1, PLAYER_BUYS) == 2200);
        CHECK(calc_store_price(1000, NEUTRAL_MARKUP, 2, PLAYER_BUYS) == 2222);
    }
}

TEST_CASE("get_black_market_multiplier grows by 1 percent per level up to the limit")
{
    // レベル1以下は2倍 (20000)。レベルが1上がるごとに1.01倍して切り捨て、500以上では変わらない
    uint64_t expected = 20000;
    for (auto level = 0; level <= 1000; ++level) {
        CAPTURE(level);
        if ((level >= 2) && (level <= 500)) {
            expected = expected * 101 / 100;
        }

        CHECK(get_black_market_multiplier(level) == expected);
    }
}
