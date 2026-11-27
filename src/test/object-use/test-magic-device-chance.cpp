/*!
 * @brief 魔道具の成否判定のテスト
 */

#include "object-use/magic-device-chance.h"
#include <doctest/doctest.h>
#include <limits>

/*!
 * @brief calc_rod_activation_weights() の通常の値と数値の境界での結果を確認する
 */
TEST_CASE("Rod and activation weights preserve normal difficulty and domain boundaries")
{
    auto weights = calc_rod_activation_weights(80, 128);
    CHECK(weights.success == 3);
    CHECK(weights.failure == 133);
    weights = calc_rod_activation_weights(80, 30);
    CHECK(weights.success == 80);
    CHECK(weights.failure == 3);
    weights = calc_rod_activation_weights(80, 100);
    CHECK(weights.success == 30);
    CHECK(weights.failure == 105);

    weights = calc_rod_activation_weights(std::numeric_limits<ACTION_SKILL_POWER>::max(), 0);
    CHECK(weights.success == 32767);
    CHECK(weights.failure == 3);
    weights = calc_rod_activation_weights(std::numeric_limits<ACTION_SKILL_POWER>::min(), 128);
    CHECK(weights.success == 3);
    CHECK(weights.failure == 133);
}

/*!
 * @brief 魔法棒・杖のレベルは 50 を超えた分だけ半分になることを確認する
 */
TEST_CASE("Wand and staff levels above 50 are halved for the excess")
{
    CHECK(calc_wand_staff_level(0) == 0);
    CHECK(calc_wand_staff_level(50) == 50);
    CHECK(calc_wand_staff_level(51) == 50);
    CHECK(calc_wand_staff_level(52) == 51);
    CHECK(calc_wand_staff_level(127) == 88);
}
