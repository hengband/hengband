#include "load/lore-loader.h"
#include "game-option/runtime-arguments.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/old/load-v1-5-0.h"
#include "load/savedata-old-flag-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "system/angband.h"
#include "system/monster-race-info.h"
#include "system/system-variables.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

static void migrate_old_feature_flags(MonsterRaceInfo *r_ptr, BIT_FLAGS old_flags)
{
    if (any_bits(old_flags, enum2i(SavedataLoreOlderThan19FlagType::RF2_PASS_WALL))) {
        r_ptr->r_feature_flags.set(MonsterFeatureType::PASS_WALL);
    }
    if (any_bits(old_flags, enum2i(SavedataLoreOlderThan19FlagType::RF2_KILL_WALL))) {
        r_ptr->r_feature_flags.set(MonsterFeatureType::KILL_WALL);
    }
}

static void migrate_old_aura_flags(MonsterRaceInfo *r_ptr)
{
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

static void migrate_old_resistance_flags(MonsterRaceInfo *r_ptr, BIT_FLAGS old_flags)
{
    struct flag_list_ver14 {
        SavedataLoreOlderThan14FlagType old_flag;
        MonsterResistanceType flag;
    };
    const std::vector<flag_list_ver14> flag_list = {
        { SavedataLoreOlderThan14FlagType::RFR_IM_ACID, MonsterResistanceType::IMMUNE_ACID },
        { SavedataLoreOlderThan14FlagType::RFR_IM_ELEC, MonsterResistanceType::IMMUNE_ELEC },
        { SavedataLoreOlderThan14FlagType::RFR_IM_FIRE, MonsterResistanceType::IMMUNE_FIRE },
        { SavedataLoreOlderThan14FlagType::RFR_IM_COLD, MonsterResistanceType::IMMUNE_COLD },
        { SavedataLoreOlderThan14FlagType::RFR_IM_POIS, MonsterResistanceType::IMMUNE_POISON },
        { SavedataLoreOlderThan14FlagType::RFR_RES_LITE, MonsterResistanceType::RESIST_LITE },
        { SavedataLoreOlderThan14FlagType::RFR_RES_DARK, MonsterResistanceType::RESIST_DARK },
        { SavedataLoreOlderThan14FlagType::RFR_RES_NETH, MonsterResistanceType::RESIST_NETHER },
        { SavedataLoreOlderThan14FlagType::RFR_RES_WATE, MonsterResistanceType::RESIST_WATER },
        { SavedataLoreOlderThan14FlagType::RFR_RES_PLAS, MonsterResistanceType::RESIST_PLASMA },
        { SavedataLoreOlderThan14FlagType::RFR_RES_SHAR, MonsterResistanceType::RESIST_SHARDS },
        { SavedataLoreOlderThan14FlagType::RFR_RES_SOUN, MonsterResistanceType::RESIST_SOUND },
        { SavedataLoreOlderThan14FlagType::RFR_RES_CHAO, MonsterResistanceType::RESIST_CHAOS },
        { SavedataLoreOlderThan14FlagType::RFR_RES_NEXU, MonsterResistanceType::RESIST_NEXUS },
        { SavedataLoreOlderThan14FlagType::RFR_RES_DISE, MonsterResistanceType::RESIST_DISENCHANT },
        { SavedataLoreOlderThan14FlagType::RFR_RES_WALL, MonsterResistanceType::RESIST_FORCE },
        { SavedataLoreOlderThan14FlagType::RFR_RES_INER, MonsterResistanceType::RESIST_INERTIA },
        { SavedataLoreOlderThan14FlagType::RFR_RES_TIME, MonsterResistanceType::RESIST_TIME },
        { SavedataLoreOlderThan14FlagType::RFR_RES_GRAV, MonsterResistanceType::RESIST_GRAVITY },
        { SavedataLoreOlderThan14FlagType::RFR_RES_ALL, MonsterResistanceType::RESIST_ALL },
        { SavedataLoreOlderThan14FlagType::RFR_RES_TELE, MonsterResistanceType::RESIST_TELEPORT }
    };

    if (old_flags == 0) {
        return;
    }

    for (const auto &f : flag_list) {
        if (any_bits(old_flags, f.old_flag)) {
            r_ptr->r_resistance_flags.set(f.flag);
        }
    }
}

static void migrate_old_drop_flags(MonsterRaceInfo *r_ptr, BIT_FLAGS old_flags1)
{
    struct flag_list_ver18 {
        SavedataLoreOlderThan18FlagType old_flag;
        MonsterDropType flag;
    };

    std::vector<flag_list_ver18> flag_list = {
        { SavedataLoreOlderThan18FlagType::RF1_ONLY_GOLD, MonsterDropType::ONLY_GOLD },
        { SavedataLoreOlderThan18FlagType::RF1_ONLY_ITEM, MonsterDropType::ONLY_ITEM },
        { SavedataLoreOlderThan18FlagType::RF1_DROP_GOOD, MonsterDropType::DROP_GOOD },
        { SavedataLoreOlderThan18FlagType::RF1_DROP_GREAT, MonsterDropType::DROP_GREAT },
        { SavedataLoreOlderThan18FlagType::RF1_DROP_60, MonsterDropType::DROP_60 },
        { SavedataLoreOlderThan18FlagType::RF1_DROP_90, MonsterDropType::DROP_90 },
        { SavedataLoreOlderThan18FlagType::RF1_DROP_1D2, MonsterDropType::DROP_1D2 },
        { SavedataLoreOlderThan18FlagType::RF1_DROP_2D2, MonsterDropType::DROP_2D2 },
        { SavedataLoreOlderThan18FlagType::RF1_DROP_3D2, MonsterDropType::DROP_3D2 },
        { SavedataLoreOlderThan18FlagType::RF1_DROP_4D2, MonsterDropType::DROP_4D2 },
    };

    for (const auto &l : flag_list) {
        if (any_bits(old_flags1, enum2i(l.old_flag))) {
            r_ptr->r_drop_flags.set(l.flag);
        }
    }
}

static void migrate_old_no_debuff_flags(MonsterRaceInfo *r_ptr)
{
    struct flag_list_ver19 {
        SavedataLoreOlderThan19FlagType_No_Debuff old_flag;
        MonsterResistanceType flag;
    };

    const std::vector<flag_list_ver19> flag_list = {
        { SavedataLoreOlderThan19FlagType_No_Debuff::RF3_NO_FEAR, MonsterResistanceType::NO_FEAR },
        { SavedataLoreOlderThan19FlagType_No_Debuff::RF3_NO_STUN, MonsterResistanceType::NO_STUN },
        { SavedataLoreOlderThan19FlagType_No_Debuff::RF3_NO_CONF, MonsterResistanceType::NO_CONF },
        { SavedataLoreOlderThan19FlagType_No_Debuff::RF3_NO_SLEEP, MonsterResistanceType::NO_SLEEP },
    };

    for (const auto &l : flag_list) {
        if (any_bits(r_ptr->r_flags3, l.old_flag)) {
            r_ptr->r_resistance_flags.set(l.flag);
        }
    }
}

static void rd_r_drop_flags(MonsterRaceInfo *r_ptr)
{
    if (loading_savefile_version_is_older_than(18)) {
        migrate_old_drop_flags(r_ptr, r_ptr->r_flags1);
        return;
    }

    rd_FlagGroup(r_ptr->r_drop_flags, rd_byte);
}

static void rd_r_ability_flags(MonsterRaceInfo *r_ptr, const MonsterRaceId r_idx)
{
    if (loading_savefile_version_is_older_than(3)) {
        BIT_FLAGS r_flagsr = 0;
        uint32_t f4 = rd_u32b();
        uint32_t f5 = rd_u32b();
        uint32_t f6 = rd_u32b();
        if (h_older_than(1, 5, 0, 3)) {
            set_old_lore(r_ptr, f4, r_idx);
        } else {
            r_flagsr = rd_u32b();
        }

        migrate_bitflag_to_flaggroup(r_ptr->r_ability_flags, f4, sizeof(uint32_t) * 8 * 0);
        migrate_bitflag_to_flaggroup(r_ptr->r_ability_flags, f5, sizeof(uint32_t) * 8 * 1);
        migrate_bitflag_to_flaggroup(r_ptr->r_ability_flags, f6, sizeof(uint32_t) * 8 * 2);

        migrate_old_resistance_flags(r_ptr, r_flagsr);
    } else if (loading_savefile_version_is_older_than(14)) {
        BIT_FLAGS r_flagsr = rd_u32b();
        rd_FlagGroup(r_ptr->r_ability_flags, rd_byte);

        migrate_old_resistance_flags(r_ptr, r_flagsr);
    } else {
        rd_FlagGroup(r_ptr->r_resistance_flags, rd_byte);
        rd_FlagGroup(r_ptr->r_ability_flags, rd_byte);
    }
}

static void rd_r_aura_flags(MonsterRaceInfo *r_ptr)
{
    if (loading_savefile_version_is_older_than(10)) {
        return;
    }

    rd_FlagGroup(r_ptr->r_aura_flags, rd_byte);
}

static void rd_r_kind_flags(MonsterRaceInfo *r_ptr)
{
    if (loading_savefile_version_is_older_than(12)) {
        struct flag_list_ver12 {
            BIT_FLAGS check_flag;
            MonsterKindType flag;
        };

        const std::vector<flag_list_ver12> flag1 = {
            { RF1_UNIQUE, MonsterKindType::UNIQUE },
        };

        const std::vector<flag_list_ver12> flag2 = {
            { static_cast<BIT_FLAGS>(RF2_HUMAN), MonsterKindType::HUMAN },
            { static_cast<BIT_FLAGS>(RF2_QUANTUM), MonsterKindType::QUANTUM },
        };

        const std::vector<flag_list_ver12> flag3 = {
            { RF3_ORC, MonsterKindType::ORC },
            { RF3_TROLL, MonsterKindType::TROLL },
            { RF3_GIANT, MonsterKindType::GIANT },
            { RF3_DRAGON, MonsterKindType::DRAGON },
            { RF3_DEMON, MonsterKindType::DEMON },
            { RF3_AMBERITE, MonsterKindType::AMBERITE },
            { RF3_ANGEL, MonsterKindType::ANGEL },
            { RF3_DRAGON, MonsterKindType::DRAGON },
            { RF3_EVIL, MonsterKindType::EVIL },
            { RF3_GOOD, MonsterKindType::GOOD },
            { RF3_ANIMAL, MonsterKindType::ANIMAL },
            { RF3_UNDEAD, MonsterKindType::UNDEAD },

        };

        for (const auto &f : flag1) {
            if (any_bits(r_ptr->r_flags1, f.check_flag)) {
                r_ptr->r_kind_flags.set(f.flag);
            }
        }

        for (const auto &f : flag2) {
            if (any_bits(r_ptr->r_flags2, f.check_flag)) {
                r_ptr->r_kind_flags.set(f.flag);
            }
        }

        for (const auto &f : flag3) {
            if (any_bits(r_ptr->r_flags3, f.check_flag)) {
                r_ptr->r_kind_flags.set(f.flag);
            }
        }

        return;
    }

    rd_FlagGroup(r_ptr->r_kind_flags, rd_byte);
}

static void rd_r_behavior_flags(MonsterRaceInfo *r_ptr)
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

        for (const auto &f : flag1) {
            if (any_bits(r_ptr->r_flags1, f.check_flag)) {
                r_ptr->r_behavior_flags.set(f.flag);
            }
        }

        for (const auto &f : flag2) {
            if (any_bits(r_ptr->r_flags2, f.check_flag)) {
                r_ptr->r_behavior_flags.set(f.flag);
            }
        }

        return;
    }

    rd_FlagGroup(r_ptr->r_behavior_flags, rd_byte);
}

static void rd_r_feature_flags(MonsterRaceInfo *r_ptr)
{
    if (loading_savefile_version_is_older_than(19)) {
        migrate_old_feature_flags(r_ptr, r_ptr->r_flags2);
        return;
    }
    rd_FlagGroup(r_ptr->r_feature_flags, rd_byte);
}

/*!
 * @brief モンスターの思い出を読み込む / Read the monster lore
 * @param r_ptr 読み込み先モンスター種族情報へのポインタ
 * @param r_idx 読み込み先モンスターID(種族特定用)
 */
static void rd_lore(MonsterRaceInfo *r_ptr, const MonsterRaceId r_idx)
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
    migrate_old_no_debuff_flags(r_ptr);
    migrate_old_aura_flags(r_ptr);
    rd_r_ability_flags(r_ptr, r_idx);
    rd_r_aura_flags(r_ptr);
    rd_r_behavior_flags(r_ptr);
    rd_r_kind_flags(r_ptr);
    rd_r_drop_flags(r_ptr);
    rd_r_feature_flags(r_ptr);
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
    r_ptr->r_resistance_flags &= r_ptr->resistance_flags;
    r_ptr->r_ability_flags &= r_ptr->ability_flags;
    r_ptr->r_aura_flags &= r_ptr->aura_flags;
    r_ptr->r_behavior_flags &= r_ptr->behavior_flags;
    r_ptr->r_drop_flags &= r_ptr->drop_flags;
    r_ptr->r_kind_flags &= r_ptr->kind_flags;
    r_ptr->r_feature_flags &= r_ptr->feature_flags;
}

void load_lore(void)
{
    auto loading_max_r_idx = rd_u16b();
    MonsterRaceInfo dummy;
    for (auto i = 0U; i < loading_max_r_idx; i++) {
        auto r_idx = static_cast<MonsterRaceId>(i);
        auto *r_ptr = i < monraces_info.size() ? &monraces_info[r_idx] : &dummy;
        rd_lore(r_ptr, r_idx);
    }

    for (size_t i = loading_max_r_idx; i < monraces_info.size(); i++) {
        auto monrace_id = i2enum<MonsterRaceId>(i);
        auto &monrace = monraces_info[monrace_id];
        auto max_num = MAX_MONSTER_NUM;
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE) || monrace.population_flags.has(MonsterPopulationType::ONLY_ONE)) {
            max_num = MAX_UNIQUE_NUM;
        } else if (monrace.population_flags.has(MonsterPopulationType::NAZGUL)) {
            max_num = MAX_NAZGUL_NUM;
        }

        monrace.max_num = max_num;
    }

    load_note(_("モンスターの思い出をロードしました", "Loaded Monster Memory"));
}
