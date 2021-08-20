#include "save/monster-writer.h"
#include "load/savedata-flag-types.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "save/save-util.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "util/quarks.h"

static void write_monster_flags(monster_type *m_ptr, BIT_FLAGS *flags)
{
    if (!is_original_ap(m_ptr))
        *flags |= SAVE_MON_AP_R_IDX;

    if (m_ptr->sub_align)
        *flags |= SAVE_MON_SUB_ALIGN;

    if (monster_csleep_remaining(m_ptr))
        *flags |= SAVE_MON_CSLEEP;

    if (monster_fast_remaining(m_ptr))
        *flags |= SAVE_MON_FAST;

    if (monster_slow_remaining(m_ptr))
        *flags |= SAVE_MON_SLOW;

    if (monster_stunned_remaining(m_ptr))
        *flags |= SAVE_MON_STUNNED;

    if (monster_confused_remaining(m_ptr))
        *flags |= SAVE_MON_CONFUSED;

    if (monster_fear_remaining(m_ptr))
        *flags |= SAVE_MON_MONFEAR;

    if (m_ptr->target_y)
        *flags |= SAVE_MON_TARGET_Y;

    if (m_ptr->target_x)
        *flags |= SAVE_MON_TARGET_X;

    if (monster_invulner_remaining(m_ptr))
        *flags |= SAVE_MON_INVULNER;

    if (m_ptr->smart.any())
        *flags |= SAVE_MON_SMART;

    if (m_ptr->exp)
        *flags |= SAVE_MON_EXP;

    if (m_ptr->mflag2.any())
        *flags |= SAVE_MON_MFLAG2;

    if (m_ptr->nickname)
        *flags |= SAVE_MON_NICKNAME;

    if (m_ptr->parent_m_idx)
        *flags |= SAVE_MON_PARENT;

    wr_u32b(*flags);
}

static void write_monster_info(monster_type *m_ptr, const BIT_FLAGS flags)
{
    byte tmp8u;
    if (flags & SAVE_MON_FAST) {
        tmp8u = (byte)m_ptr->mtimed[MTIMED_FAST];
        wr_byte(tmp8u);
    }

    if (flags & SAVE_MON_SLOW) {
        tmp8u = (byte)m_ptr->mtimed[MTIMED_SLOW];
        wr_byte(tmp8u);
    }

    if (flags & SAVE_MON_STUNNED) {
        tmp8u = (byte)m_ptr->mtimed[MTIMED_STUNNED];
        wr_byte(tmp8u);
    }

    if (flags & SAVE_MON_CONFUSED) {
        tmp8u = (byte)m_ptr->mtimed[MTIMED_CONFUSED];
        wr_byte(tmp8u);
    }

    if (flags & SAVE_MON_MONFEAR) {
        tmp8u = (byte)m_ptr->mtimed[MTIMED_MONFEAR];
        wr_byte(tmp8u);
    }

    if (flags & SAVE_MON_TARGET_Y)
        wr_s16b((int16_t)m_ptr->target_y);

    if (flags & SAVE_MON_TARGET_X)
        wr_s16b((int16_t)m_ptr->target_x);

    if (flags & SAVE_MON_INVULNER) {
        tmp8u = (byte)m_ptr->mtimed[MTIMED_INVULNER];
        wr_byte(tmp8u);
    }

    if (flags & SAVE_MON_SMART)
        wr_FlagGroup(m_ptr->smart, wr_byte);

    if (flags & SAVE_MON_EXP)
        wr_u32b(m_ptr->exp);

    if (flags & SAVE_MON_MFLAG2)
        wr_FlagGroup(m_ptr->mflag2, wr_byte);

    if (flags & SAVE_MON_NICKNAME)
        wr_string(quark_str(m_ptr->nickname));

    if (flags & SAVE_MON_PARENT)
        wr_s16b(m_ptr->parent_m_idx);
}

/*!
 * @brief モンスター情報を書き込む / Write a "monster" record
 * @param m_ptr モンスター情報保存元ポインタ
 */
void wr_monster(monster_type *m_ptr)
{
    BIT_FLAGS flags = 0x00000000;
    write_monster_flags(m_ptr, &flags);

    wr_s16b(m_ptr->r_idx);
    wr_byte((byte)m_ptr->fy);
    wr_byte((byte)m_ptr->fx);
    wr_s16b((int16_t)m_ptr->hp);
    wr_s16b((int16_t)m_ptr->maxhp);
    wr_s16b((int16_t)m_ptr->max_maxhp);
    wr_u32b(m_ptr->dealt_damage);

    if (flags & SAVE_MON_AP_R_IDX)
        wr_s16b(m_ptr->ap_r_idx);

    if (flags & SAVE_MON_SUB_ALIGN)
        wr_byte(m_ptr->sub_align);

    if (flags & SAVE_MON_CSLEEP)
        wr_s16b(m_ptr->mtimed[MTIMED_CSLEEP]);

    wr_byte((byte)m_ptr->mspeed);
    wr_s16b(m_ptr->energy_need);
    write_monster_info(m_ptr, flags);
}

/*!
 * @brief モンスターの思い出を書き込む / Write a "lore" record
 * @param r_idx モンスター種族ID
 */
void wr_lore(MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    wr_s16b((int16_t)r_ptr->r_sights);
    wr_s16b((int16_t)r_ptr->r_deaths);
    wr_s16b((int16_t)r_ptr->r_pkills);
    wr_s16b((int16_t)r_ptr->r_akills);
    wr_s16b((int16_t)r_ptr->r_tkills);

    wr_byte(r_ptr->r_wake);
    wr_byte(r_ptr->r_ignore);

    wr_byte(r_ptr->r_xtra1);
    wr_byte(r_ptr->r_xtra2);

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
    wr_u32b(r_ptr->r_flagsr);
    wr_FlagGroup(r_ptr->r_ability_flags, wr_byte);

    wr_byte((byte)r_ptr->max_num);
    wr_s16b(r_ptr->floor_id);

    wr_s16b(r_ptr->defeat_level);
    wr_u32b(r_ptr->defeat_time);
    wr_byte(0);
}
