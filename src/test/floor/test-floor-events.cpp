#include "floor/floor-events.h"
#include <doctest/doctest.h>
#include <limits>

/*!
 * @brief 対象モジュールの数値境界と通常値の計算結果を確認する
 */
TEST_CASE("floor_rating_boost preserves thresholds and bounds extreme contributions")
{
    CHECK(floor_rating_boost(300) == 105000);
    CHECK(floor_rating_boost(1000) == 1050000);
    CHECK(floor_rating_boost(1001) > floor_rating_boost(1000));
    CHECK(floor_rating_boost(std::numeric_limits<int64_t>::max()) == floor_rating_boost(1001));
    CHECK(floor_rating_boost(-1) == 0);
}
