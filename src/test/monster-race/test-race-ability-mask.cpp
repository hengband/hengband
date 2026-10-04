/*!
 * @brief モンスターの能力の分類 (race-ability-mask) のテスト
 *
 * ダメージを直接与える攻撃魔法のマスク (RF_ABILITY_DAMAGE_MASK) が、ボルト・ビーム・ボール・ブレスと
 * 精神攻撃・傷の呪い系だけを含むことを検証する。
 */

#include "monster-race/race-ability-mask.h"

#include "monster-race/race-ability-flags.h"
#include "util/enum-range.h"

#include <doctest/doctest.h>

TEST_CASE("RF_ABILITY_DAMAGE_MASK contains every bolt, beam, ball and breath")
{
    for (const auto ability : EnumRange(MonsterAbilityType::SHRIEK, MonsterAbilityType::MAX)) {
        CAPTURE(ability);
        // ボールのマスクはブレスを含む
        const auto is_projection = RF_ABILITY_BOLT_MASK.has(ability) || RF_ABILITY_BEAM_MASK.has(ability) || RF_ABILITY_BALL_MASK.has(ability);
        if (is_projection) {
            CHECK(RF_ABILITY_DAMAGE_MASK.has(ability));
        }
    }
}

TEST_CASE("RF_ABILITY_DAMAGE_MASK contains the mind attacks and the cause wounds spells")
{
    for (const auto ability : { MonsterAbilityType::MIND_BLAST, MonsterAbilityType::BRAIN_SMASH,
             MonsterAbilityType::CAUSE_1, MonsterAbilityType::CAUSE_2, MonsterAbilityType::CAUSE_3, MonsterAbilityType::CAUSE_4 }) {
        CAPTURE(ability);
        CHECK(RF_ABILITY_DAMAGE_MASK.has(ability));
    }
}

TEST_CASE("RF_ABILITY_DAMAGE_MASK contains nothing else")
{
    // ボルト 16 種 (ロケットと射撃を含む)・ビーム 1 種・ボール 15 種・ブレス 24 種と、精神攻撃・傷の呪い系 6 種
    CHECK(RF_ABILITY_DAMAGE_MASK.count() == 62);

    for (const auto ability : { MonsterAbilityType::DRAIN_MANA, MonsterAbilityType::SCARE, MonsterAbilityType::HAND_DOOM,
             MonsterAbilityType::HEAL, MonsterAbilityType::BLINK, MonsterAbilityType::S_KIN }) {
        CAPTURE(ability);
        CHECK_FALSE(RF_ABILITY_DAMAGE_MASK.has(ability));
    }
}
