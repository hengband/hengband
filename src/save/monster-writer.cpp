#include "save/monster-writer.h"
#include "load/old/monster-flag-types-savefile50.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "save/save-util.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/quarks.h"

static void write_monster_flags(monster_type *m_ptr, BIT_FLAGS *flags)
{
    if (!is_original_ap(m_ptr)) {
        set_bits(*flags, SaveDataMonsterFlagType::AP_R_IDX);
    }

    if (m_ptr->sub_align) {
        set_bits(*flags, SaveDataMonsterFlagType::SUB_ALIGN);
    }

    if (monster_csleep_remaining(m_ptr)) {
        set_bits(*flags, SaveDataMonsterFlagType::CSLEEP);
    }

    if (monster_fast_remaining(m_ptr)) {
        set_bits(*flags, SaveDataMonsterFlagType::FAST);
    }

    if (monster_slow_remaining(m_ptr)) {
        set_bits(*flags, SaveDataMonsterFlagType::SLOW);
    }

    if (monster_stunned_remaining(m_ptr)) {
        set_bits(*flags, SaveDataMonsterFlagType::STUNNED);
    }

    if (monster_confused_remaining(m_ptr)) {
        set_bits(*flags, SaveDataMonsterFlagType::CONFUSED);
    }

    if (monster_fear_remaining(m_ptr)) {
        set_bits(*flags, SaveDataMonsterFlagType::MONFEAR);
    }

    if (m_ptr->target_y) {
        set_bits(*flags, SaveDataMonsterFlagType::TARGET_Y);
    }

    if (m_ptr->target_x) {
        set_bits(*flags, SaveDataMonsterFlagType::TARGET_X);
    }

    if (monster_invulner_remaining(m_ptr)) {
        set_bits(*flags, SaveDataMonsterFlagType::INVULNER);
    }

    if (m_ptr->smart.any()) {
        set_bits(*flags, SaveDataMonsterFlagType::SMART);
    }

    if (m_ptr->exp) {
        set_bits(*flags, SaveDataMonsterFlagType::EXP);
    }

    if (m_ptr->mflag2.any()) {
        set_bits(*flags, SaveDataMonsterFlagType::MFLAG2);
    }

    if (m_ptr->nickname) {
        set_bits(*flags, SaveDataMonsterFlagType::NICKNAME);
    }

    if (m_ptr->parent_m_idx) {
        set_bits(*flags, SaveDataMonsterFlagType::PARENT);
    }

    wr_u32b(*flags);
}

static void write_monster_info(monster_type *m_ptr, const BIT_FLAGS flags)
{
    byte tmp8u;
    if (any_bits(flags, SaveDataMonsterFlagType::FAST)) {
        tmp8u = (byte)m_ptr->mtimed[MTIMED_FAST];
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SLOW)) {
        tmp8u = (byte)m_ptr->mtimed[MTIMED_SLOW];
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::STUNNED)) {
        tmp8u = (byte)m_ptr->mtimed[MTIMED_STUNNED];
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::CONFUSED)) {
        tmp8u = (byte)m_ptr->mtimed[MTIMED_CONFUSED];
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::MONFEAR)) {
        tmp8u = (byte)m_ptr->mtimed[MTIMED_MONFEAR];
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::TARGET_Y)) {
        wr_s16b((int16_t)m_ptr->target_y);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::TARGET_X)) {
        wr_s16b((int16_t)m_ptr->target_x);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::INVULNER)) {
        tmp8u = (byte)m_ptr->mtimed[MTIMED_INVULNER];
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SMART)) {
        wr_FlagGroup(m_ptr->smart, wr_byte);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::EXP)) {
        wr_u32b(m_ptr->exp);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::MFLAG2)) {
        wr_FlagGroup(m_ptr->mflag2, wr_byte);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::NICKNAME)) {
        wr_string(quark_str(m_ptr->nickname));
    }

    if (any_bits(flags, SaveDataMonsterFlagType::PARENT)) {
        wr_s16b(m_ptr->parent_m_idx);
    }
}

/*!
 * @brief モンスター情報を書き込む / Write a "monster" record
 * @param m_ptr モンスター情報保存元ポインタ
 */
void wr_monster(monster_type *m_ptr)
{
    BIT_FLAGS flags = 0x00000000;
    write_monster_flags(m_ptr, &flags);

    wr_s16b(enum2i(m_ptr->r_idx));
    wr_byte((byte)m_ptr->fy);
    wr_byte((byte)m_ptr->fx);
    wr_s16b((int16_t)m_ptr->hp);
    wr_s16b((int16_t)m_ptr->maxhp);
    wr_s16b((int16_t)m_ptr->max_maxhp);
    wr_u32b(m_ptr->dealt_damage);

    if (any_bits(flags, SaveDataMonsterFlagType::AP_R_IDX)) {
        wr_s16b(enum2i(m_ptr->ap_r_idx));
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SUB_ALIGN)) {
        wr_byte(m_ptr->sub_align);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::CSLEEP)) {
        wr_s16b(m_ptr->mtimed[MTIMED_CSLEEP]);
    }

    wr_byte((byte)m_ptr->mspeed);
    wr_s16b(m_ptr->energy_need);
    write_monster_info(m_ptr, flags);
}

/*!
 * @brief モンスターの思い出を書き込む / Write a "lore" record
 * @param r_idx モンスター種族ID
 */
void wr_lore(MonsterRaceId r_idx)
{
    auto *r_ptr = &r_info[r_idx];
    wr_s16b((int16_t)r_ptr->r_sights);
    wr_s16b((int16_t)r_ptr->r_deaths);
    wr_s16b((int16_t)r_ptr->r_pkills);
    wr_s16b((int16_t)r_ptr->r_akills);
    wr_s16b((int16_t)r_ptr->r_tkills);

    wr_byte(r_ptr->r_wake);
    wr_byte(r_ptr->r_ignore);

    byte tmp8u = r_ptr->r_can_evolve ? 1 : 0;
    wr_byte(tmp8u);

    wr_byte((byte)r_ptr->r_drop_gold);
    wr_byte((byte)r_ptr->r_drop_item);

    wr_byte(0); /* unused now */
    wr_byte(r_ptr->r_cast_spell);

    wr_byte(r_ptr->r_blows[0]);
    wr_byte(r_ptr->r_blows[1]);
    wr_byte(r_ptr->r_blows[2]);
    wr_byte(r_ptr->r_blows[3]);

    wr_u32b(r_ptr->r_flags1);
    wr_u32b(r_ptr->r_flags2);
    wr_u32b(r_ptr->r_flags3);
    wr_FlagGroup(r_ptr->r_resistance_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_ability_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_aura_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_behavior_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_kind_flags, wr_byte);

    wr_byte((byte)r_ptr->max_num);
    wr_s16b(r_ptr->floor_id);

    wr_s16b(r_ptr->defeat_level);
    wr_u32b(r_ptr->defeat_time);
    wr_byte(0);
}
