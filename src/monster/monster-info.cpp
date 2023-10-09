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
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"

/*!
 * @brief モンスターを友好的にする
 * @param m_ptr モンスター情報構造体の参照ポインタ
 */
void set_friendly(MonsterEntity *m_ptr)
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
bool monster_can_cross_terrain(PlayerType *player_ptr, FEAT_IDX feat, const MonsterRaceInfo *r_ptr, BIT_FLAGS16 mode)
{
    auto *f_ptr = &terrains_info[feat];

    if (f_ptr->flags.has(TerrainCharacteristics::PATTERN)) {
        if (!(mode & CEM_RIDING)) {
            if (r_ptr->feature_flags.has_not(MonsterFeatureType::CAN_FLY)) {
                return false;
            }
        } else {
            if (!(mode & CEM_P_CAN_ENTER_PATTERN)) {
                return false;
            }
        }
    }

    if (f_ptr->flags.has(TerrainCharacteristics::CAN_FLY) && r_ptr->feature_flags.has(MonsterFeatureType::CAN_FLY)) {
        return true;
    }
    if (f_ptr->flags.has(TerrainCharacteristics::CAN_SWIM) && r_ptr->feature_flags.has(MonsterFeatureType::CAN_SWIM)) {
        return true;
    }
    if (f_ptr->flags.has(TerrainCharacteristics::CAN_PASS)) {
        if (r_ptr->feature_flags.has(MonsterFeatureType::PASS_WALL) && (!(mode & CEM_RIDING) || has_pass_wall(player_ptr))) {
            return true;
        }
    }

    if (f_ptr->flags.has_not(TerrainCharacteristics::MOVE)) {
        return false;
    }

    if (f_ptr->flags.has(TerrainCharacteristics::MOUNTAIN) && (r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN))) {
        return true;
    }

    if (f_ptr->flags.has(TerrainCharacteristics::WATER)) {
        if (r_ptr->feature_flags.has_not(MonsterFeatureType::AQUATIC)) {
            if (f_ptr->flags.has(TerrainCharacteristics::DEEP)) {
                return false;
            } else if (r_ptr->aura_flags.has(MonsterAuraType::FIRE)) {
                return false;
            }
        }
    } else if (r_ptr->feature_flags.has(MonsterFeatureType::AQUATIC)) {
        return false;
    }

    if (f_ptr->flags.has(TerrainCharacteristics::LAVA)) {
        if (r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_FIRE_MASK)) {
            return false;
        }
    }

    if (f_ptr->flags.has(TerrainCharacteristics::COLD_PUDDLE)) {
        if (r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_COLD_MASK)) {
            return false;
        }
    }

    if (f_ptr->flags.has(TerrainCharacteristics::ELEC_PUDDLE)) {
        if (r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_ELEC_MASK)) {
            return false;
        }
    }

    if (f_ptr->flags.has(TerrainCharacteristics::ACID_PUDDLE)) {
        if (r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_ACID_MASK)) {
            return false;
        }
    }

    if (f_ptr->flags.has(TerrainCharacteristics::POISON_PUDDLE)) {
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
bool monster_can_enter(PlayerType *player_ptr, POSITION y, POSITION x, MonsterRaceInfo *r_ptr, BIT_FLAGS16 mode)
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
 * @param monster1 モンスター1への参照
 * @param monster2 モンスター2への参照
 * @return 敵対関係にあるならばTRUEを返す
 */
bool are_enemies(PlayerType *player_ptr, const MonsterEntity &monster1, const MonsterEntity &monster2)
{
    if (player_ptr->phase_out) {
        if (monster1.is_pet() || monster2.is_pet()) {
            return false;
        }
        return true;
    }

    const auto &monrace1 = monster1.get_monrace();
    const auto &monrace2 = monster2.get_monrace();
    const auto is_m1_wild = monrace1.wilderness_flags.has_any_of({ MonsterWildernessType::WILD_TOWN, MonsterWildernessType::WILD_ALL });
    const auto is_m2_wild = monrace2.wilderness_flags.has_any_of({ MonsterWildernessType::WILD_TOWN, MonsterWildernessType::WILD_ALL });
    if (is_m1_wild && is_m2_wild) {
        if (!monster1.is_pet() && !monster2.is_pet()) {
            return false;
        }
    }

    if (check_hostile_align(monster1.sub_align, monster2.sub_align)) {
        if (monster1.mflag2.has_not(MonsterConstantFlagType::CHAMELEON) || monster2.mflag2.has_not(MonsterConstantFlagType::CHAMELEON)) {
            return true;
        }
    }

    if (monster1.is_hostile() != monster2.is_hostile()) {
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
bool monster_has_hostile_align(PlayerType *player_ptr, MonsterEntity *m_ptr, int pa_good, int pa_evil, MonsterRaceInfo *r_ptr)
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

bool is_original_ap_and_seen(PlayerType *player_ptr, const MonsterEntity *m_ptr)
{
    return m_ptr->ml && !player_ptr->effects()->hallucination()->is_hallucinated() && m_ptr->is_original_ap();
}

/*!
 * @brief モンスターIDを取り、モンスター名をm_nameに代入する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @return std::string モンスター名
 */
std::string monster_name(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    return monster_desc(player_ptr, m_ptr, 0x00);
}
