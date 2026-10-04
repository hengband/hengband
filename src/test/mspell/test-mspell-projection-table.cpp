/*!
 * @brief プレイヤーが使うモンスター魔法のブレス・ボール・ボルトの表のテスト
 *
 * 青魔法とものまねが共通で引く表について、モンスター側の能力の分類 (race-ability-mask) と突き合わせて、
 * ブレス・ボール・ボルトがすべて引けること、放ち方とボールの半径が正しいこと、それ以外の魔法は引けないことを検証する。
 */

#include "mspell/mspell-projection-table.h"

#include "monster-race/race-ability-flags.h"
#include "monster-race/race-ability-mask.h"
#include "util/enum-range.h"

#include <doctest/doctest.h>

namespace {
/*!
 * @brief モンスター側の分類から、期待する放ち方を求める
 * @details ロケットと射撃はボール・ボルトの分類に含まれるが、専用の処理で放つので表には無い
 */
tl::optional<MspellProjectionType> expected_type(MonsterAbilityType ability)
{
    if ((ability == MonsterAbilityType::ROCKET) || (ability == MonsterAbilityType::SHOOT)) {
        return tl::nullopt;
    }

    if (RF_ABILITY_BREATH_MASK.has(ability)) {
        return MspellProjectionType::BREATH;
    }

    if (RF_ABILITY_BALL_MASK.has(ability)) {
        return MspellProjectionType::BALL;
    }

    if (RF_ABILITY_BOLT_MASK.has(ability)) {
        return MspellProjectionType::BOLT;
    }

    return tl::nullopt;
}
}

TEST_CASE("find_mspell_projection matches the monster ability classification")
{
    auto count = 0;
    for (const auto ability : EnumRange(MonsterAbilityType::SHRIEK, MonsterAbilityType::MAX)) {
        CAPTURE(ability);
        const auto projection = find_mspell_projection(ability);
        const auto type = expected_type(ability);
        REQUIRE(projection.has_value() == type.has_value());
        if (!projection) {
            continue;
        }

        ++count;
        CHECK(projection->type == *type);
        CHECK_FALSE(projection->message.empty());
        if (projection->type == MspellProjectionType::BALL) {
            CHECK(projection->radius == (RF_ABILITY_BIG_BALL_MASK.has(ability) ? 4 : 2));
        }
    }

    // ブレス 24 種・ボール 15 種・ボルト 14 種
    CHECK(count == 53);
}
