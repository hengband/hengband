#include "load/monster-loader.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-v1-5-0.h"
#include "load/savedata-flag-types.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
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

    auto flags = rd_u32b();
    m_ptr->r_idx = rd_s16b();
    m_ptr->fy = rd_byte();
    m_ptr->fx = rd_byte();

    m_ptr->hp = rd_s16b();
    m_ptr->maxhp = rd_s16b();
    m_ptr->max_maxhp = rd_s16b();

    if (h_older_than(2, 1, 2, 1)) {
        m_ptr->dealt_damage = 0;
    } else {
        m_ptr->dealt_damage = rd_s32b();
    }

    if (any_bits(flags, SaveDataMonsterFlagType::AP_R_IDX))
        m_ptr->ap_r_idx = rd_s16b();
    else
        m_ptr->ap_r_idx = m_ptr->r_idx;

    if (any_bits(flags, SaveDataMonsterFlagType::SUB_ALIGN))
        m_ptr->sub_align = rd_byte();
    else
        m_ptr->sub_align = 0;

    if (any_bits(flags, SaveDataMonsterFlagType::CSLEEP))
        m_ptr->mtimed[MTIMED_CSLEEP] = rd_s16b();
    else
        m_ptr->mtimed[MTIMED_CSLEEP] = 0;

    m_ptr->mspeed = rd_byte();

    m_ptr->energy_need = rd_s16b();

    if (any_bits(flags, SaveDataMonsterFlagType::FAST)) {
        m_ptr->mtimed[MTIMED_FAST] = rd_byte();
    } else
        m_ptr->mtimed[MTIMED_FAST] = 0;

    if (any_bits(flags, SaveDataMonsterFlagType::SLOW)) {
        m_ptr->mtimed[MTIMED_SLOW] = rd_byte();
    } else
        m_ptr->mtimed[MTIMED_SLOW] = 0;

    if (any_bits(flags, SaveDataMonsterFlagType::STUNNED)) {
        m_ptr->mtimed[MTIMED_STUNNED] = rd_byte();
    } else
        m_ptr->mtimed[MTIMED_STUNNED] = 0;

    if (any_bits(flags, SaveDataMonsterFlagType::CONFUSED)) {
        m_ptr->mtimed[MTIMED_CONFUSED] = rd_byte();
    } else
        m_ptr->mtimed[MTIMED_CONFUSED] = 0;

    if (any_bits(flags, SaveDataMonsterFlagType::MONFEAR)) {
        m_ptr->mtimed[MTIMED_MONFEAR] = rd_byte();
    } else
        m_ptr->mtimed[MTIMED_MONFEAR] = 0;

    if (any_bits(flags, SaveDataMonsterFlagType::TARGET_Y)) {
        m_ptr->target_y = rd_s16b();
    } else
        m_ptr->target_y = 0;

    if (any_bits(flags, SaveDataMonsterFlagType::TARGET_X)) {
        m_ptr->target_x = rd_s16b();
    } else
        m_ptr->target_x = 0;

    if (any_bits(flags, SaveDataMonsterFlagType::INVULNER)) {
        m_ptr->mtimed[MTIMED_INVULNER] = rd_byte();
    } else
        m_ptr->mtimed[MTIMED_INVULNER] = 0;

    m_ptr->mflag.clear();
    m_ptr->mflag2.clear();

    if (any_bits(flags, SaveDataMonsterFlagType::SMART)) {
        if (loading_savefile_version_is_older_than(2)) {
            auto tmp32u = rd_u32b();
            migrate_bitflag_to_flaggroup(m_ptr->smart, tmp32u);

            // 3.0.0Alpha10以前のSM_CLONED(ビット位置22)、SM_PET(23)、SM_FRIEDLY(28)をMFLAG2に移行する
            // ビット位置の定義はなくなるので、ビット位置の値をハードコードする。
            std::bitset<32> rd_bits(tmp32u);
            m_ptr->mflag2[MFLAG2::CLONED] = rd_bits[22];
            m_ptr->mflag2[MFLAG2::PET] = rd_bits[23];
            m_ptr->mflag2[MFLAG2::FRIENDLY] = rd_bits[28];
            m_ptr->smart.reset(i2enum<SM>(22)).reset(i2enum<SM>(23)).reset(i2enum<SM>(28));
        } else {
            rd_FlagGroup(m_ptr->smart, rd_byte);
        }
    } else {
        m_ptr->smart.clear();
    }

    if (any_bits(flags, SaveDataMonsterFlagType::EXP)) {
        m_ptr->exp = rd_u32b();
    } else
        m_ptr->exp = 0;

    if (any_bits(flags, SaveDataMonsterFlagType::MFLAG2)) {
        if (loading_savefile_version_is_older_than(2)) {
            auto tmp8u = rd_byte();
            constexpr auto base = enum2i(MFLAG2::KAGE);
            migrate_bitflag_to_flaggroup(m_ptr->mflag2, tmp8u, base, 7);
        } else {
            rd_FlagGroup(m_ptr->mflag2, rd_byte);
        }
    }

    if (any_bits(flags, SaveDataMonsterFlagType::NICKNAME)) {
        char buf[128];
        rd_string(buf, sizeof(buf));
        m_ptr->nickname = quark_add(buf);
    } else
        m_ptr->nickname = 0;

    if (any_bits(flags, SaveDataMonsterFlagType::PARENT))
        m_ptr->parent_m_idx = rd_s16b();
    else
        m_ptr->parent_m_idx = 0;
}
