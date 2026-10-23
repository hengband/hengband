/*!
 * @brief 施設の定義のテスト
 *
 * モンスター闘技場の払い戻しが、倍率の高いモンスターに大きく賭けてもあふれないことを検証する。
 */

#include "system/building-type-definition.h"

#include <doctest/doctest.h>
#include <limits>

TEST_CASE("MeleeArena::calc_payback() keeps the payback for ordinary odds")
{
    CHECK(MeleeArena::calc_payback(100, 250) == 250);
    CHECK(MeleeArena::calc_payback(100, 105) == 105);
    CHECK(MeleeArena::calc_payback(1, 105) == 2);
}

TEST_CASE("MeleeArena::calc_payback() does not overflow with high odds and a large wager")
{
    SUBCASE("the payback fits in PRICE")
    {
        CHECK(MeleeArena::calc_payback(10000, 415000) == 41500000);
    }

    SUBCASE("the payback exceeds PRICE")
    {
        CHECK(MeleeArena::calc_payback(10000, std::numeric_limits<int>::max()) == std::numeric_limits<int>::max());
    }
}
