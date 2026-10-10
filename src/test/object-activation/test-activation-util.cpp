#include "object-activation/activation-util.h"
#include <doctest/doctest.h>
#include <limits>

/*!
 * @brief 対象モジュールの数値境界と通常値の計算結果を確認する
 */
TEST_CASE("Activation weights preserve normal difficulty and domain boundaries")
{
    ae_type activation;
    activation.lev = 128;
    activation.decide_chance_fail(80);
    CHECK(activation.chance == 3);
    CHECK(activation.fail == 133);
    activation.lev = 30;
    activation.decide_chance_fail(80);
    CHECK(activation.chance == 80);
    CHECK(activation.fail == 3);
    activation.lev = 100;
    activation.decide_chance_fail(80);
    CHECK(activation.chance == 30);
    CHECK(activation.fail == 105);

    activation.lev = 0;
    activation.decide_chance_fail(std::numeric_limits<ACTION_SKILL_POWER>::max());
    CHECK(activation.chance == 32767);
    CHECK(activation.fail == 3);
    activation.lev = 128;
    activation.decide_chance_fail(std::numeric_limits<ACTION_SKILL_POWER>::min());
    CHECK(activation.chance == 3);
    CHECK(activation.fail == 133);
}
