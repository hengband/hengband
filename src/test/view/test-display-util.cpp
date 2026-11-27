/*!
 * @brief 表示の補助のテスト
 */

#include "view/display-util.h"
#include <doctest/doctest.h>

#ifdef JP
TEST_CASE("format_item_weight shows the weight in kilograms")
{
    // 重量は 0.1 ポンド単位で、日本語版は 1 ポンドを 0.5 kg として換算する
    CHECK(format_item_weight(130) == "  6.5 kg");
    CHECK(format_item_weight(7) == "  0.3 kg");
    CHECK(format_item_weight(0) == "  0.0 kg");
    CHECK(format_item_weight(30000) == "1500.0 kg");
}

TEST_CASE("format_item_weight can omit the unit")
{
    CHECK(format_item_weight(130, false) == "  6.5");
}
#else
TEST_CASE("format_item_weight shows the weight in pounds")
{
    // 重量は 0.1 ポンド単位
    CHECK(format_item_weight(130) == " 13.0 lb");
    CHECK(format_item_weight(7) == "  0.7 lb");
    CHECK(format_item_weight(0) == "  0.0 lb");
    CHECK(format_item_weight(30000) == "3000.0 lb");
}

TEST_CASE("format_item_weight can omit the unit")
{
    CHECK(format_item_weight(130, false) == " 13.0");
}
#endif
