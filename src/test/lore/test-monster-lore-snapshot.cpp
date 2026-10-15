/*!
 * @brief 思い出の表示を決める値の写しのテスト
 */

#include "game-option/cheat-options.h"
#include "game-option/text-display-options.h"
#include "lore/monster-lore-snapshot.h"
#include "system/monrace/monrace-records.h"
#include "system/player-type-definition.h"
#include "test/scoped-restore.h"
#include "test/system/monrace-records-test-access.h"
#include "world/world.h"
#include <doctest/doctest.h>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

TEST_CASE("MonsterLoreSnapshot compares the values shown in the monster recall")
{
    test::MonraceRecordsTestAccess records;
    const auto restore_options = test::scoped_restore(cheat_know);
    cheat_know = false;
    auto player_ptr = std::make_unique<PlayerType>();
    auto &player = *player_ptr;
    player.lev = 10;
    player.chp = 100;
    MonraceDefinition monrace;
    monrace.idx = static_cast<MonraceId>(1);
    const MonsterLoreSnapshot snapshot(player, monrace);
    CHECK(snapshot == MonsterLoreSnapshot(player, monrace));

    SUBCASE("Each lore value is compared")
    {
        // 初期化子の写し間違いに気づけるよう、値を 1 つずつ変えて比べる
        const std::vector<std::pair<std::string_view, std::function<void(MonraceDefinition &)>>> changes{
            { "r_deaths", [](auto &r) { r.r_deaths++; } },
            { "r_pkills", [](auto &r) { r.r_pkills++; } },
            { "r_akills", [](auto &r) { r.r_akills++; } },
            { "r_tkills", [](auto &r) { r.r_tkills++; } },
            { "r_wake", [](auto &r) { r.r_wake++; } },
            { "r_ignore", [](auto &r) { r.r_ignore++; } },
            { "r_can_evolve", [](auto &r) { r.r_can_evolve = true; } },
            { "r_drop_gold", [](auto &r) { r.r_drop_gold++; } },
            { "r_drop_item", [](auto &r) { r.r_drop_item++; } },
            { "r_cast_spell", [](auto &r) { r.r_cast_spell++; } },
            { "r_blows", [](auto &r) { r.r_blows[MAX_NUM_BLOWS - 1]++; } },
            { "r_ability_flags", [](auto &r) { r.r_ability_flags.set(MonsterAbilityType::SHRIEK); } },
            { "r_aura_flags", [](auto &r) { r.r_aura_flags.set(MonsterAuraType::FIRE); } },
            { "r_behavior_flags", [](auto &r) { r.r_behavior_flags.set(MonsterBehaviorType::NEVER_MOVE); } },
            { "r_resistance_flags", [](auto &r) { r.r_resistance_flags.set(MonsterResistanceType::RESIST_FIRE); } },
            { "r_drop_flags", [](auto &r) { r.r_drop_flags.set(MonsterDropType::DROP_GOOD); } },
            { "r_feature_flags", [](auto &r) { r.r_feature_flags.set(MonsterFeatureType::CAN_FLY); } },
            { "r_special_flags", [](auto &r) { r.r_special_flags.set(MonsterSpecialType::DIMINISH_MAX_DAMAGE); } },
            { "r_misc_flags", [](auto &r) { r.r_misc_flags.set(MonsterMiscType::HAS_FRIENDS); } },
            { "max_num", [](auto &r) { r.max_num++; } },
        };
        MonraceDefinition other_race;
        other_race.idx = static_cast<MonraceId>(2);
        CHECK(snapshot != MonsterLoreSnapshot(player, other_race));

        for (const auto &[name, change] : changes) {
            CAPTURE(name);
            MonraceDefinition changed;
            changed.idx = monrace.idx;
            change(changed);
            CHECK(snapshot != MonsterLoreSnapshot(player, changed));
        }
    }

    SUBCASE("Each value of the player and the options is compared")
    {
        auto &world = AngbandWorld::get_instance();
        const auto restore_world = test::scoped_restore(world.wizard, show_lore_summary, depth_in_feet);
        const std::vector<std::pair<std::string_view, std::function<void(bool)>>> changes{
            { "lev", [&player, old = player.lev](bool is_changed) { player.lev = is_changed ? static_cast<PLAYER_LEVEL>(old + 1) : old; } },
            { "max_plv", [&player, old = player.max_plv](bool is_changed) { player.max_plv = is_changed ? static_cast<int16_t>(old + 1) : old; } },
            { "wizard", [&world](bool is_changed) { world.wizard = is_changed; } },
            { "cheat_know", [](bool is_changed) { cheat_know = is_changed; } },
            { "show_lore_summary", [](bool is_changed) { show_lore_summary = is_changed; } },
            { "depth_in_feet", [](bool is_changed) { depth_in_feet = is_changed; } },
        };
        world.wizard = false;
        show_lore_summary = false;
        depth_in_feet = false;
        const MonsterLoreSnapshot snapshot_default(player, monrace);
        for (const auto &[name, change] : changes) {
            CAPTURE(name);
            change(true);
            CHECK(snapshot_default != MonsterLoreSnapshot(player, monrace));
            change(false);
            CHECK(snapshot_default == MonsterLoreSnapshot(player, monrace));
        }
    }

    SUBCASE("The kind flags are not shown, so they are not compared")
    {
        monrace.r_kind_flags.set(MonsterKindType::EVIL);
        CHECK(snapshot == MonsterLoreSnapshot(player, monrace));
    }

    SUBCASE("The sightings are compared")
    {
        MonraceRecords::get_instance().increment_seen_count(monrace.idx);
        CHECK(snapshot != MonsterLoreSnapshot(player, monrace));
    }

    SUBCASE("The hit points are not compared even for a race with the hand of doom")
    {
        // 思い出の破滅の手の威力は固定の文言で、HP によらない
        monrace.ability_flags.set(MonsterAbilityType::HAND_DOOM);
        monrace.r_ability_flags.set(MonsterAbilityType::HAND_DOOM);
        monrace.r_cast_spell = MAX_UCHAR;
        const MonsterLoreSnapshot snapshot_doom(player, monrace);
        player.chp = 50;
        CHECK(snapshot_doom == MonsterLoreSnapshot(player, monrace));
    }
}
