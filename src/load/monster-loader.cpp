#include "load/monster-loader.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-v1-5-0.h"
#include "load/savedata-flag-types.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "util/quarks.h"

/*!
 * @brief モンスターを読み込む(現版) / Read a monster (New method)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_ptr モンスター保存先ポインタ
 */
void rd_monster(player_type *player_ptr, monster_type *m_ptr)
{
    if (h_older_than(1, 5, 0, 0)) {
        rd_monster_old(player_ptr, m_ptr);
        return;
    }

    BIT_FLAGS flags;
    rd_u32b(&flags);
    rd_s16b(&m_ptr->r_idx);
    byte tmp8u;
    rd_byte(&tmp8u);
    m_ptr->fy = (POSITION)tmp8u;
    rd_byte(&tmp8u);
    m_ptr->fx = (POSITION)tmp8u;

    int16_t tmp16s;
    rd_s16b(&tmp16s);
    m_ptr->hp = (HIT_POINT)tmp16s;
    rd_s16b(&tmp16s);
    m_ptr->maxhp = (HIT_POINT)tmp16s;
    rd_s16b(&tmp16s);
    m_ptr->max_maxhp = (HIT_POINT)tmp16s;

    if (h_older_than(2, 1, 2, 1)) {
        m_ptr->dealt_damage = 0;
    } else {
        rd_s32b(&m_ptr->dealt_damage);
    }

    if (flags & SAVE_MON_AP_R_IDX)
        rd_s16b(&m_ptr->ap_r_idx);
    else
        m_ptr->ap_r_idx = m_ptr->r_idx;

    if (flags & SAVE_MON_SUB_ALIGN)
        rd_byte(&m_ptr->sub_align);
    else
        m_ptr->sub_align = 0;

    if (flags & SAVE_MON_CSLEEP)
        rd_s16b(&m_ptr->mtimed[MTIMED_CSLEEP]);
    else
        m_ptr->mtimed[MTIMED_CSLEEP] = 0;

    rd_byte(&tmp8u);
    m_ptr->mspeed = tmp8u;

    rd_s16b(&m_ptr->energy_need);

    if (flags & SAVE_MON_FAST) {
        rd_byte(&tmp8u);
        m_ptr->mtimed[MTIMED_FAST] = (int16_t)tmp8u;
    } else
        m_ptr->mtimed[MTIMED_FAST] = 0;

    if (flags & SAVE_MON_SLOW) {
        rd_byte(&tmp8u);
        m_ptr->mtimed[MTIMED_SLOW] = (int16_t)tmp8u;
    } else
        m_ptr->mtimed[MTIMED_SLOW] = 0;

    if (flags & SAVE_MON_STUNNED) {
        rd_byte(&tmp8u);
        m_ptr->mtimed[MTIMED_STUNNED] = (int16_t)tmp8u;
    } else
        m_ptr->mtimed[MTIMED_STUNNED] = 0;

    if (flags & SAVE_MON_CONFUSED) {
        rd_byte(&tmp8u);
        m_ptr->mtimed[MTIMED_CONFUSED] = (int16_t)tmp8u;
    } else
        m_ptr->mtimed[MTIMED_CONFUSED] = 0;

    if (flags & SAVE_MON_MONFEAR) {
        rd_byte(&tmp8u);
        m_ptr->mtimed[MTIMED_MONFEAR] = (int16_t)tmp8u;
    } else
        m_ptr->mtimed[MTIMED_MONFEAR] = 0;

    if (flags & SAVE_MON_TARGET_Y) {
        rd_s16b(&tmp16s);
        m_ptr->target_y = (POSITION)tmp16s;
    } else
        m_ptr->target_y = 0;

    if (flags & SAVE_MON_TARGET_X) {
        rd_s16b(&tmp16s);
        m_ptr->target_x = (POSITION)tmp16s;
    } else
        m_ptr->target_x = 0;

    if (flags & SAVE_MON_INVULNER) {
        rd_byte(&tmp8u);
        m_ptr->mtimed[MTIMED_INVULNER] = (int16_t)tmp8u;
    } else
        m_ptr->mtimed[MTIMED_INVULNER] = 0;

    m_ptr->mflag.clear();
    m_ptr->mflag2.clear();

    if (flags & SAVE_MON_SMART) {
        if (loading_savefile_version_is_older_than(2)) {
            uint32_t tmp32u;
            rd_u32b(&tmp32u);
            migrate_bitflag_to_flaggroup(m_ptr->smart, tmp32u);

            // 3.0.0Alpha10以前のSM_CLONED(ビット位置22)、SM_PET(23)、SM_FRIEDLY(28)をMFLAG2に移行する
            // ビット位置の定義はなくなるので、ビット位置の値をハードコードする。
            std::bitset<32> rd_bits(tmp32u);
            m_ptr->mflag2[MFLAG2::CLONED] = rd_bits[22];
            m_ptr->mflag2[MFLAG2::PET] = rd_bits[23];
            m_ptr->mflag2[MFLAG2::FRIENDLY] = rd_bits[28];
            m_ptr->smart.reset(static_cast<SM>(22)).reset(static_cast<SM>(23)).reset(static_cast<SM>(28));
        } else {
            rd_FlagGroup(m_ptr->smart, rd_byte);
        }
    } else {
        m_ptr->smart.clear();
    }

    if (flags & SAVE_MON_EXP) {
        uint32_t tmp32u;
        rd_u32b(&tmp32u);
        m_ptr->exp = (EXP)tmp32u;
    } else
        m_ptr->exp = 0;

    if (flags & SAVE_MON_MFLAG2) {
        if (loading_savefile_version_is_older_than(2)) {
            rd_byte(&tmp8u);
            constexpr auto base = enum2i(MFLAG2::KAGE);
            migrate_bitflag_to_flaggroup(m_ptr->mflag2, tmp8u, base, 7);
        } else {
            rd_FlagGroup(m_ptr->mflag2, rd_byte);
        }
    }

    if (flags & SAVE_MON_NICKNAME) {
        char buf[128];
        rd_string(buf, sizeof(buf));
        m_ptr->nickname = quark_add(buf);
    } else
        m_ptr->nickname = 0;

    if (flags & SAVE_MON_PARENT)
        rd_s16b(&m_ptr->parent_m_idx);
    else
        m_ptr->parent_m_idx = 0;
}
