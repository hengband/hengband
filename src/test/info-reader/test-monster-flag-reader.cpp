/*!
 * @brief モンスターの定義とダンジョンの出現条件に共通するフラグ名の読み込み (monster-flag-reader) のテスト
 *
 * 各読み込み処理からの呼び出しは、test-race-reader.cpp・test-dungeon-reader.cpp で扱う。
 */

#include "info-reader/monster-flag-reader.h"
#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"
#include "view/display-messages.h"
#include <doctest/doctest.h>

TEST_CASE("grab_one_monster_spell_flag sets a flag found in the ability flag table")
{
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    ScopedMessageCapture diagnostics;
    CHECK(grab_one_monster_spell_flag(ability_flags, "BR_FIRE"));
    CHECK(ability_flags.has(MonsterAbilityType::BR_FIRE));
    CHECK(ability_flags.count() == 1);
    CHECK(diagnostics.messages().empty());
}

TEST_CASE("grab_one_monster_spell_flag keeps the flags already set")
{
    EnumClassFlagGroup<MonsterAbilityType> ability_flags{ MonsterAbilityType::SHRIEK };
    CHECK(grab_one_monster_spell_flag(ability_flags, "BR_FIRE"));
    CHECK(ability_flags.has(MonsterAbilityType::SHRIEK));
    CHECK(ability_flags.has(MonsterAbilityType::BR_FIRE));
    CHECK(ability_flags.count() == 2);
}

TEST_CASE("grab_one_monster_spell_flag rejects an unknown name and prints it once")
{
    for (const auto *name : { "UNKNOWN_FLAG", "", "br_fire", "BR_FIRE ", "NEVER_MOVE" }) {
        CAPTURE(name);
        EnumClassFlagGroup<MonsterAbilityType> ability_flags{ MonsterAbilityType::SHRIEK };
        ScopedMessageCapture diagnostics;
        CHECK_FALSE(grab_one_monster_spell_flag(ability_flags, name));
        CHECK(ability_flags.has(MonsterAbilityType::SHRIEK));
        CHECK(ability_flags.count() == 1);
        CHECK(diagnostics.messages().size() == 1);
    }
}

#ifndef JP
TEST_CASE("grab_one_monster_spell_flag reports the unknown name in the monster flag message")
{
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    ScopedMessageCapture diagnostics;
    CHECK_FALSE(grab_one_monster_spell_flag(ability_flags, "UNKNOWN_FLAG"));
    REQUIRE(diagnostics.messages().size() == 1);
    CHECK(diagnostics.messages()[0] == "Unknown monster flag 'UNKNOWN_FLAG'.");
}
#endif
