/*!
 * @brief AC による打撃のダメージの軽減のテスト
 */

#include "combat/ac-damage-reduction.h"
#include <doctest/doctest.h>

TEST_CASE("reduce_damage_by_ac reduces damage in proportion to AC")
{
    CHECK(reduce_damage_by_ac(100, 0) == 100);
    CHECK(reduce_damage_by_ac(100, 50) == 80);
    CHECK(reduce_damage_by_ac(100, 150) == 40);
    CHECK(reduce_damage_by_ac(37, 100) == 23); // 端数は軽減の分を切り捨てる
}

TEST_CASE("reduce_damage_by_ac caps AC at 150")
{
    CHECK(reduce_damage_by_ac(100, 149) == 41);
    CHECK(reduce_damage_by_ac(100, 151) == 40);
    CHECK(reduce_damage_by_ac(100, 300) == 40);
}

TEST_CASE("reduce_damage_by_ac increases damage with negative AC")
{
    CHECK(reduce_damage_by_ac(100, -50) == 120);
}
