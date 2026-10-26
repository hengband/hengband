/*!
 * @brief 薬を飲んだ時の処理のテスト
 *
 * 水分を栄養にする種族 (エント) の満腹度の計算が回り込まないことを検証する。
 */

#include "object-use/quaff/quaff-execution.h"
#include "player/digestion-processor.h"

#include <doctest/doctest.h>

TEST_CASE("calc_moistened_food() keeps the result for ordinary potions")
{
    CHECK(calc_moistened_food(0, 0) == 2000);
    CHECK(calc_moistened_food(1000, 100) == 4100);
    CHECK(calc_moistened_food(5000, -100) == 6900);
    CHECK(calc_moistened_food(14000, 100) == PY_FOOD_MAX - 1);
}

TEST_CASE("calc_moistened_food() does not wrap around with large nutrition")
{
    SUBCASE("ten times the nutrition exceeds short")
    {
        CHECK(calc_moistened_food(0, 3277) == PY_FOOD_MAX - 1);
    }

    SUBCASE("the sum exceeds short when the player is gorged")
    {
        CHECK(calc_moistened_food(20000, 979) == PY_FOOD_MAX - 1);
    }
}

TEST_CASE("calc_moistened_food() does not wrap around with negative nutrition")
{
    CHECK(calc_moistened_food(5000, -10000) == 0);
    CHECK(calc_moistened_food(20000, -10000) == 12000);
}
