/*!
 * @brief モンスター種族定義 (MonraceDefinition) のテスト
 *
 * 調査 (probe_lore()) で思い出にオーラが記録されることを検証する。
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
