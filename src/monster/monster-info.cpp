/*!
 * @brief モンスター情報の記述 / describe monsters (using monster memory)
 * @date 2013/12/11
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "monster/monster-info.h"
#include "floor/cave.h"
#include "floor/wild.h"
#include "grid/feature.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "player/player-status-flags.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"

/*!
 * @brief モンスターを友好的にする
 * @param m_ptr モンスター情報構造体の参照ポインタ
 */
void set_friendly(monster_type *m_ptr)
{
    m_ptr->mflag2.set(MonsterConstantFlagType::FRIENDLY);
}

/*!
 * @brief モンスターが地形を踏破できるかどうかを返す
 * Check if monster can cross terrain
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param feat 地形ID
 * @param r_ptr モンスター種族構造体の参照ポインタ
 * @param mode オプション
 * @return 踏破可能ならばTRUEを返す
 */
bool monster_can_cross_terrain(PlayerType *player_ptr, FEAT_IDX feat, monster_race *r_ptr, BIT_FLAGS16 mode)
{
    auto *f_ptr = &f_info[feat];

    if (f_ptr->flags.has(FloorFeatureType::PATTERN)) {
        if (!(mode & CEM_RIDING)) {
            if (!(r_ptr->flags7 & RF7_CAN_FLY)) {
                return false;
            }
        } else {
            if (!(mode & CEM_P_CAN_ENTER_PATTERN)) {
                return false;
            }
        }
    }

    if (f_ptr->flags.has(FloorFeatureType::CAN_FLY) && (r_ptr->flags7 & RF7_CAN_FLY)) {
        return true;
    }
    if (f_ptr->flags.has(FloorFeatureType::CAN_SWIM) && (r_ptr->flags7 & RF7_CAN_SWIM)) {
        return true;
    }
    if (f_ptr->flags.has(FloorFeatureType::CAN_PASS)) {
        if ((r_ptr->flags2 & RF2_PASS_WALL) && (!(mode & CEM_RIDING) || has_pass_wall(player_ptr))) {
            return true;
        }
    }

    if (f_ptr->flags.has_not(FloorFeatureType::MOVE)) {
        return false;
    }

    if (f_ptr->flags.has(FloorFeatureType::MOUNTAIN) && (r_ptr->flags8 & RF8_WILD_MOUNTAIN)) {
        return true;
    }

    if (f_ptr->flags.has(FloorFeatureType::WATER)) {
        if (!(r_ptr->flags7 & RF7_AQUATIC)) {
            if (f_ptr->flags.has(FloorFeatureType::DEEP)) {
                return false;
            } else if (r_ptr->aura_flags.has(MonsterAuraType::FIRE)) {
                return false;
            }
        }
    } else if (r_ptr->flags7 & RF7_AQUATIC) {
        return false;
    }

    if (f_ptr->flags.has(FloorFeatureType::LAVA)) {
        if (r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_FIRE_MASK)) {
            return false;
        }
    }

    if (f_ptr->flags.has(FloorFeatureType::COLD_PUDDLE)) {
        if (r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_COLD_MASK)) {
            return false;
        }
    }

    if (f_ptr->flags.has(FloorFeatureType::ELEC_PUDDLE)) {
        if (r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_ELEC_MASK)) {
            return false;
        }
    }

    if (f_ptr->flags.has(FloorFeatureType::ACID_PUDDLE)) {
        if (r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_ACID_MASK)) {
            return false;
        }
    }

    if (f_ptr->flags.has(FloorFeatureType::POISON_PUDDLE)) {
        if (r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_POISON_MASK)) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief 指定された座標の地形をモンスターが踏破できるかどうかを返す
 * Strictly check if monster can enter the grid
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 地形のY座標
 * @param x 地形のX座標
 * @param r_ptr モンスター種族構造体の参照ポインタ
 * @param mode オプション
 * @return 踏破可能ならばTRUEを返す
 */
bool monster_can_enter(PlayerType *player_ptr, POSITION y, POSITION x, monster_race *r_ptr, BIT_FLAGS16 mode)
{
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
    if (player_bold(player_ptr, y, x)) {
        return false;
    }
    if (g_ptr->m_idx) {
        return false;
    }

    return monster_can_cross_terrain(player_ptr, g_ptr->feat, r_ptr, mode);
}

/*!
 * @brief モンスターの属性の基づいた敵対関係の有無を返す（サブルーチン）
 * Check if this monster has "hostile" alignment (aux)
 * @param sub_align1 モンスター1のサブフラグ
 * @param sub_align2 モンスター2のサブフラグ
 * @return 敵対関係にあるならばTRUEを返す
 */
static bool check_hostile_align(byte sub_align1, byte sub_align2)
{
    if (sub_align1 != sub_align2) {
        if (((sub_align1 & SUB_ALIGN_EVIL) && (sub_align2 & SUB_ALIGN_GOOD)) || ((sub_align1 & SUB_ALIGN_GOOD) && (sub_align2 & SUB_ALIGN_EVIL))) {
            return true;
        }
    }

    return false;
}

/*!
 * @brief モンスターの属性の基づいた敵対関係の有無を返す
 * Check if two monsters are enemies
 * @param m_ptr モンスター1の構造体参照ポインタ
 * @param n_ptr モンスター2の構造体参照ポインタ
 * @return 敵対関係にあるならばTRUEを返す
 */
bool are_enemies(PlayerType *player_ptr, monster_type *m_ptr, monster_type *n_ptr)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    monster_race *s_ptr = &r_info[n_ptr->r_idx];

    if (player_ptr->phase_out) {
        if (is_pet(m_ptr) || is_pet(n_ptr)) {
            return false;
        }
        return true;
    }

    if ((r_ptr->flags8 & (RF8_WILD_TOWN | RF8_WILD_ALL)) && (s_ptr->flags8 & (RF8_WILD_TOWN | RF8_WILD_ALL))) {
        if (!is_pet(m_ptr) && !is_pet(n_ptr)) {
            return false;
        }
    }

    if (check_hostile_align(m_ptr->sub_align, n_ptr->sub_align)) {
        if (m_ptr->mflag2.has_not(MonsterConstantFlagType::CHAMELEON) || n_ptr->mflag2.has_not(MonsterConstantFlagType::CHAMELEON)) {
            return true;
        }
    }

    if (is_hostile(m_ptr) != is_hostile(n_ptr)) {
        return true;
    }

    return false;
}

/*!
 * @brief モンスターがプレイヤーに対して敵意を抱くかどうかを返す
 * Check if this monster race has "hostile" alignment
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @param pa_good プレイヤーの善傾向値
 * @param pa_evil プレイヤーの悪傾向値
 * @param r_ptr モンスター種族情報の構造体参照ポインタ
 * @return プレイヤーに敵意を持つならばTRUEを返す
 * @details
 * If user is player, m_ptr == nullptr.
 */
bool monster_has_hostile_align(PlayerType *player_ptr, monster_type *m_ptr, int pa_good, int pa_evil, monster_race *r_ptr)
{
    byte sub_align1 = SUB_ALIGN_NEUTRAL;
    byte sub_align2 = SUB_ALIGN_NEUTRAL;

    if (m_ptr) /* For a monster */
    {
        sub_align1 = m_ptr->sub_align;
    } else /* For player */
    {
        if (player_ptr->alignment >= pa_good) {
            sub_align1 |= SUB_ALIGN_GOOD;
        }
        if (player_ptr->alignment <= pa_evil) {
            sub_align1 |= SUB_ALIGN_EVIL;
        }
    }

    /* Racial alignment flags */
    if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
        sub_align2 |= SUB_ALIGN_EVIL;
    }
    if (r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
        sub_align2 |= SUB_ALIGN_GOOD;
    }

    if (check_hostile_align(sub_align1, sub_align2)) {
        return true;
    }

    return false;
}

bool is_original_ap_and_seen(PlayerType *player_ptr, monster_type *m_ptr)
{
    return m_ptr->ml && !player_ptr->hallucinated && (m_ptr->ap_r_idx == m_ptr->r_idx);
}

/*  Determine monster race appearance index is same as race index */
bool is_original_ap(const monster_type *m_ptr)
{
    return m_ptr->ap_r_idx == m_ptr->r_idx;
}

bool is_friendly(const monster_type *m_ptr)
{
    return m_ptr->mflag2.has(MonsterConstantFlagType::FRIENDLY);
}

bool is_pet(const monster_type *m_ptr)
{
    return m_ptr->mflag2.has(MonsterConstantFlagType::PET);
}

bool is_hostile(const monster_type *m_ptr)
{
    return !is_friendly(m_ptr) && !is_pet(m_ptr);
}

/*!
 * @brief モンスターがアイテム類に擬態しているかどうかを返す
 *
 * モンスターがアイテム類に擬態しているかどうかを返す。
 * 擬態の条件:
 * - シンボルが以下のいずれかであること: /|\()[]=$,.!?&`#%<>+~
 * - 動かない、もしくは眠っていること
 *
 * 但し、以下のモンスターは例外的に擬態しているとする
 * それ・生ける虚無『ヌル』・ビハインダー
 *
 * @param m_ptr モンスターの参照ポインタ
 * @return モンスターがアイテム類に擬態しているならTRUE、そうでなければFALSE
 */
bool is_mimicry(monster_type *m_ptr)
{
    if (m_ptr->ap_r_idx == MON_IT || m_ptr->ap_r_idx == MON_NULL || m_ptr->ap_r_idx == MON_BEHINDER) {
        return true;
    }

    auto *r_ptr = &r_info[m_ptr->ap_r_idx];

    if (angband_strchr("/|\\()[]=$,.!?&`#%<>+~", r_ptr->d_char) == nullptr) {
        return false;
    }

    if (r_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_MOVE) && !monster_csleep_remaining(m_ptr)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターの真の種族を返す / Extract monster race pointer of a monster's true form
 * @param m_ptr モンスターの参照ポインタ
 * @return 本当のモンスター種族参照ポインタ
 */
monster_race *real_r_ptr(monster_type *m_ptr)
{
    return &r_info[real_r_idx(m_ptr)];
}

MONRACE_IDX real_r_idx(monster_type *m_ptr)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (m_ptr->mflag2.has(MonsterConstantFlagType::CHAMELEON)) {
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            return MON_CHAMELEON_K;
        } else {
            return MON_CHAMELEON;
        }
    }

    return m_ptr->r_idx;
}

/*!
 * @brief モンスターIDを取り、モンスター名をm_nameに代入する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param m_name モンスター名を入力する配列
 */
void monster_name(PlayerType *player_ptr, MONSTER_IDX m_idx, char *m_name)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    monster_desc(player_ptr, m_name, m_ptr, 0x00);
}
