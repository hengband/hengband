/*!
 * @brief モンスター種族定義 (MonraceDefinition) のテスト
 *
 * 調査 (probe_lore()) で思い出にオーラが記録されることと、
 * 闘技場の強さ (calc_power()) が大きな値でもあふれないことを検証する。
 */

#include "system/monrace/monrace-definition.h"

#include <doctest/doctest.h>

TEST_CASE("MonraceDefinition::probe_lore() records auras")
{
    MonraceDefinition monrace;
    monrace.aura_flags.set(MonsterAuraType::FIRE).set(MonsterAuraType::ELEC);

    CHECK(monrace.probe_lore().has_value());
    CHECK(monrace.r_aura_flags == monrace.aura_flags);
}

TEST_CASE("MonraceDefinition::probe_lore() reports unknown auras as new information")
{
    MonraceDefinition monrace;
    monrace.aura_flags.set(MonsterAuraType::COLD);
    (void)monrace.probe_lore();

    SUBCASE("nothing is new after probing once")
    {
        CHECK_FALSE(monrace.probe_lore().has_value());
    }

    SUBCASE("an aura unknown to the lore is new")
    {
        monrace.r_aura_flags.clear();
        CHECK(monrace.probe_lore().has_value());
        CHECK(monrace.r_aura_flags.has(MonsterAuraType::COLD));
    }
}

namespace {
/*!
 * @brief 同梱の「メタルバブル」と同じ値で強さを計算するモンスター種族を作る
 * @details 3d2・レベル80・加速+80・全属性耐性で、闘技場の強さは430万になる。
 */
MonraceDefinition make_metal_babble_like()
{
    MonraceDefinition monrace;
    monrace.hit_dice = Dice(3, 2);
    monrace.level = 80;
    monrace.speed = 190;
    monrace.resistance_flags.set(MonsterResistanceType::RESIST_ALL);
    return monrace;
}
}

TEST_CASE("MonraceDefinition::calc_power() keeps the value without odds correction")
{
    const auto monrace = make_metal_babble_like();
    CHECK(monrace.calc_power() == 4300000);
}

TEST_CASE("MonraceDefinition::calc_power() does not overflow with a large odds correction")
{
    auto monrace = make_metal_babble_like();

    SUBCASE("a ratio that kept within int before")
    {
        monrace.arena_ratio = 150;
        CHECK(monrace.calc_power() == 6450000);
    }

    SUBCASE("the smallest ratio that overflowed int before")
    {
        monrace.arena_ratio = 500;
        CHECK(monrace.calc_power() == 21500000);
    }

    SUBCASE("the largest ratio accepted by the reader")
    {
        monrace.arena_ratio = 1000;
        CHECK(monrace.calc_power() == 43000000);
    }
}
