#include "load/lore-loader.h"
#include "game-option/runtime-arguments.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/old/load-v1-5-0.h"
#include "load/savedata-old-flag-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "system/angband.h"
#include "system/monster-race-definition.h"
#include "util/bit-flags-calculator.h"

static void migrate_old_aura_flags(monster_race *r_ptr)
{
    if (loading_savefile_version_is_older_than(10)) {
        if (any_bits(r_ptr->r_flags2, SavedataLoreOlderThan10FlagType::AURA_FIRE_OLD)) {
            r_ptr->r_aura_flags.set(MonsterAuraType::FIRE);
        }

        if (any_bits(r_ptr->r_flags3, SavedataLoreOlderThan10FlagType::AURA_COLD_OLD)) {
            r_ptr->r_aura_flags.set(MonsterAuraType::COLD);
        }

        if (any_bits(r_ptr->r_flags2, SavedataLoreOlderThan10FlagType::AURA_ELEC_OLD)) {
            r_ptr->r_aura_flags.set(MonsterAuraType::ELEC);
        }
    }
}

static void rd_r_ability_flags(monster_race *r_ptr, const MONRACE_IDX r_idx)
{
    if (loading_savefile_version_is_older_than(3)) {
        uint32_t f4 = rd_u32b();
        uint32_t f5 = rd_u32b();
        uint32_t f6 = rd_u32b();
        if (h_older_than(1, 5, 0, 3))
            set_old_lore(r_ptr, f4, r_idx);
        else
            r_ptr->r_flagsr = rd_u32b();

        migrate_bitflag_to_flaggroup(r_ptr->r_ability_flags, f4, sizeof(uint32_t) * 8 * 0);
        migrate_bitflag_to_flaggroup(r_ptr->r_ability_flags, f5, sizeof(uint32_t) * 8 * 1);
        migrate_bitflag_to_flaggroup(r_ptr->r_ability_flags, f6, sizeof(uint32_t) * 8 * 2);
    } else {
        r_ptr->r_flagsr = rd_u32b();
        rd_FlagGroup(r_ptr->r_ability_flags, rd_byte);
    }
}

static void rd_r_aura_flags(monster_race *r_ptr)
{
    if (loading_savefile_version_is_older_than(10)) {
        return;
    }

    rd_FlagGroup(r_ptr->r_aura_flags, rd_byte);
}

static void rd_r_behavior_flags(monster_race *r_ptr)
{
    if (loading_savefile_version_is_older_than(11)) {
        struct flag_list_ver11 {
            BIT_FLAGS check_flag;
            MonsterBehaviorType flag;
        };

        const std::vector<flag_list_ver11> flag1 = {
            { RF1_NEVER_BLOW, MonsterBehaviorType::NEVER_BLOW },
            { RF1_NEVER_MOVE, MonsterBehaviorType::NEVER_MOVE },
            { RF1_RAND_25, MonsterBehaviorType::RAND_MOVE_25 },
            { RF1_RAND_50, MonsterBehaviorType::RAND_MOVE_50 },
        };

        const std::vector<flag_list_ver11> flag2 = {
            { RF2_OPEN_DOOR, MonsterBehaviorType::OPEN_DOOR },
            { RF2_BASH_DOOR, MonsterBehaviorType::BASH_DOOR },
            { RF2_MOVE_BODY, MonsterBehaviorType::MOVE_BODY },
            { RF2_KILL_BODY, MonsterBehaviorType::KILL_BODY },
            { RF2_TAKE_ITEM, MonsterBehaviorType::TAKE_ITEM },
            { RF2_KILL_ITEM, MonsterBehaviorType::KILL_ITEM },
            { RF2_STUPID, MonsterBehaviorType::STUPID },
            { RF2_SMART, MonsterBehaviorType::SMART },
        };

        for (const auto &f : flag1)
            if (any_bits(r_ptr->r_flags1, f.check_flag))
                r_ptr->r_behavior_flags.set(f.flag);

        for (const auto &f : flag2)
            if (any_bits(r_ptr->r_flags2, f.check_flag))
                r_ptr->r_behavior_flags.set(f.flag);

        return;
    }

    rd_FlagGroup(r_ptr->r_behavior_flags, rd_byte);
}

/*!
 * @brief モンスターの思い出を読み込む / Read the monster lore
 * @param r_ptr 読み込み先モンスター種族情報へのポインタ
 * @param r_idx 読み込み先モンスターID(種族特定用)
 */
static void rd_lore(monster_race *r_ptr, const MONRACE_IDX r_idx)
{
    r_ptr->r_sights = rd_s16b();
    r_ptr->r_deaths = rd_s16b();
    r_ptr->r_pkills = rd_s16b();

    if (h_older_than(1, 7, 0, 5)) {
        r_ptr->r_akills = r_ptr->r_pkills;
    } else {
        r_ptr->r_akills = rd_s16b();
    }

    r_ptr->r_tkills = rd_s16b();

    r_ptr->r_wake = rd_byte();
    r_ptr->r_ignore = rd_byte();

    r_ptr->r_can_evolve = rd_byte() > 0;
    if (loading_savefile_version_is_older_than(6)) {
        // かつては未使用フラグr_ptr->r_xtra2だった.
        strip_bytes(1);
    }

    r_ptr->r_drop_gold = rd_byte();
    r_ptr->r_drop_item = rd_byte();

    strip_bytes(1);
    r_ptr->r_cast_spell = rd_byte();

    r_ptr->r_blows[0] = rd_byte();
    r_ptr->r_blows[1] = rd_byte();
    r_ptr->r_blows[2] = rd_byte();
    r_ptr->r_blows[3] = rd_byte();

    r_ptr->r_flags1 = rd_u32b();
    r_ptr->r_flags2 = rd_u32b();
    r_ptr->r_flags3 = rd_u32b();
    migrate_old_aura_flags(r_ptr);
    rd_r_ability_flags(r_ptr, r_idx);
    rd_r_aura_flags(r_ptr);
    rd_r_behavior_flags(r_ptr);
    r_ptr->max_num = rd_byte();
    r_ptr->floor_id = rd_s16b();

    if (!loading_savefile_version_is_older_than(4)) {
        r_ptr->defeat_level = rd_s16b();
        r_ptr->defeat_time = rd_u32b();
    }

    strip_bytes(1);

    r_ptr->r_flags1 &= r_ptr->flags1;
    r_ptr->r_flags2 &= r_ptr->flags2;
    r_ptr->r_flags3 &= r_ptr->flags3;
    r_ptr->r_flagsr &= r_ptr->flagsr;
    r_ptr->r_ability_flags &= r_ptr->ability_flags;
    r_ptr->r_aura_flags &= r_ptr->aura_flags;
    r_ptr->r_behavior_flags &= r_ptr->r_behavior_flags;
}

void load_lore(void)
{
    auto loading_max_r_idx = rd_u16b();
    monster_race dummy;
    for (auto i = 0U; i < loading_max_r_idx; i++) {
        auto *r_ptr = i < r_info.size() ? &r_info[i] : &dummy;
        rd_lore(r_ptr, static_cast<MONRACE_IDX>(i));
    }

    load_note(_("モンスターの思い出をロードしました", "Loaded Monster Memory"));
}
