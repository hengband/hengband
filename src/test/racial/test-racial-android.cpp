#include "racial/racial-android.h"
#include <doctest/doctest.h>
#include <initializer_list>
#include <limits>

/*!
 * @brief 対象モジュールの数値境界と通常値の計算結果を確認する
 */
TEST_CASE("android_item_experience preserves ordinary and bounded extreme values")
{
    CHECK(android_item_experience(100000, 35, true) == 31250000);
    CHECK(android_item_experience(100, 10, false) == 1000);
    CHECK(android_item_experience(5000000, 167, true) == 2004062500);
    CHECK(android_item_experience(1, 167, true) == 0);
}

/*!
 * @brief 対象モジュールの数値境界と通常値の計算結果を確認する
 */
TEST_CASE("android_item_experience treats negative levels as zero")
{
    for (const auto special : { false, true }) {
        CHECK(android_item_experience(5000000, -1, special) == 0);
        CHECK(android_item_experience(5000000, std::numeric_limits<int64_t>::min(), special) == 0);
        CHECK(android_item_experience(5000000, 0, special) == 0);
    }
}
