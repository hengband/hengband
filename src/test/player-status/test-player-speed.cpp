/*!
 * @brief プレイヤーの速度の計算のテスト
 *
 * 所持品の重量による減速が、大量に持っても加速に回り込まないことを検証する。
 */

#include "player-status/player-speed.h"

#include <cstdint>
#include <doctest/doctest.h>
#include <limits>

TEST_CASE("calc_speed_bonus_by_weight() keeps the slowdown for ordinary weights")
{
    CHECK(calc_speed_bonus_by_weight(500, 500) == 0);
    CHECK(calc_speed_bonus_by_weight(599, 500) == 0);
    CHECK(calc_speed_bonus_by_weight(600, 500) == -1);
    CHECK(calc_speed_bonus_by_weight(1499, 1000) == -2);
    CHECK(calc_speed_bonus_by_weight(500 + 100 * 32768, 500) == std::numeric_limits<int16_t>::min());
}

TEST_CASE("calc_speed_bonus_by_weight() does not turn a huge slowdown into a speed bonus")
{
    CHECK(calc_speed_bonus_by_weight(500 + 100 * 32769, 500) == std::numeric_limits<int16_t>::min());
    CHECK(calc_speed_bonus_by_weight(100000000, 500) == std::numeric_limits<int16_t>::min());
}
