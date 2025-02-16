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
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "player/player-status-flags.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"

/*!
 * @brief モンスターが地形を踏破できるかどうかを返す
 * Check if monster can cross terrain
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param feat 地形ID
 * @param r_ptr モンスター種族構造体の参照ポインタ
 * @param mode オプション
 * @return 踏破可能ならばTRUEを返す
 */
bool monster_can_cross_terrain(PlayerType *player_ptr, FEAT_IDX feat, const MonraceDefinition &monrace, BIT_FLAGS16 mode)
{
    const auto &terrain = TerrainList::get_instance().get_terrain(feat);
    if (terrain.flags.has(TerrainCharacteristics::PATTERN)) {
        if (!(mode & CEM_RIDING)) {
            if (monrace.feature_flags.has_not(MonsterFeatureType::CAN_FLY)) {
                return false;
            }
        } else {
            if (!(mode & CEM_P_CAN_ENTER_PATTERN)) {
                return false;
            }
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::CAN_FLY) && monrace.feature_flags.has(MonsterFeatureType::CAN_FLY)) {
        return true;
    }
    if (terrain.flags.has(TerrainCharacteristics::CAN_SWIM) && monrace.feature_flags.has(MonsterFeatureType::CAN_SWIM)) {
        return true;
    }
    if (terrain.flags.has(TerrainCharacteristics::CAN_PASS)) {
        if (monrace.feature_flags.has(MonsterFeatureType::PASS_WALL) && (!(mode & CEM_RIDING) || has_pass_wall(player_ptr))) {
            return true;
        }
    }

    if (terrain.flags.has_not(TerrainCharacteristics::MOVE)) {
        return false;
    }

    if (terrain.flags.has(TerrainCharacteristics::MOUNTAIN) && (monrace.wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN))) {
        return true;
    }

    if (terrain.flags.has(TerrainCharacteristics::WATER)) {
        if (monrace.feature_flags.has_not(MonsterFeatureType::AQUATIC)) {
            if (terrain.flags.has(TerrainCharacteristics::DEEP)) {
                return false;
            } else if (monrace.aura_flags.has(MonsterAuraType::FIRE)) {
                return false;
            }
        }
    } else if (monrace.feature_flags.has(MonsterFeatureType::AQUATIC)) {
        return false;
    }

    if (terrain.flags.has(TerrainCharacteristics::LAVA)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_IM_FIRE_MASK)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::COLD_PUDDLE)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_IM_COLD_MASK)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::ELEC_PUDDLE)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_IM_ELEC_MASK)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::ACID_PUDDLE)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_IM_ACID_MASK)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::POISON_PUDDLE)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_IM_POISON_MASK)) {
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
bool monster_can_enter(PlayerType *player_ptr, POSITION y, POSITION x, const MonraceDefinition &monrace, BIT_FLAGS16 mode)
{
    const Pos2D pos(y, x);
    auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    if (player_ptr->is_located_at(pos)) {
        return false;
    }
    if (grid.has_monster()) {
        return false;
    }

    return monster_can_cross_terrain(player_ptr, grid.feat, monrace, mode);
}

static uint8_t get_recial_sub_align(const MonraceDefinition &monrace)
{
    uint8_t sub_align = SUB_ALIGN_NEUTRAL;
    if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
        sub_align |= SUB_ALIGN_EVIL;
    }
    if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
        sub_align |= SUB_ALIGN_GOOD;
    }
    return sub_align;
}

/*!
 * @brief モンスターがプレイヤーに対して敵意を抱くかどうかを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_good プレイヤーの善傾向値
 * @param pa_evil プレイヤーの悪傾向値
 * @param monrace モンスター種族情報の参照
 * @return プレイヤーに敵意を持つならばtrueを返す
 */
bool monster_has_hostile_to_player(PlayerType *player_ptr, int pa_good, int pa_evil, const MonraceDefinition &monrace)
{
    byte sub_align1 = SUB_ALIGN_NEUTRAL;
    if (player_ptr->alignment >= pa_good) {
        sub_align1 |= SUB_ALIGN_GOOD;
    }
    if (player_ptr->alignment <= pa_evil) {
        sub_align1 |= SUB_ALIGN_EVIL;
    }

    const auto sub_align2 = get_recial_sub_align(monrace);
    return MonsterEntity::check_sub_alignments(sub_align1, sub_align2);
}

/*!
 * @brief モンスターが他のモンスターに対して敵意を抱くかどうかを返す
 * @param monster_other 敵意を抱くか調べる他のモンスターの参照
 * @param monrace モンスター種族情報の参照
 * @return monster_other で指定したモンスターに敵意を持つならばtrueを返す
 */
bool monster_has_hostile_to_other_monster(const MonsterEntity &monster_other, const MonraceDefinition &monrace)
{
    const auto sub_align2 = get_recial_sub_align(monrace);
    return MonsterEntity::check_sub_alignments(monster_other.sub_align, sub_align2);
}

bool is_original_ap_and_seen(PlayerType *player_ptr, const MonsterEntity &monster)
{
    return monster.ml && !player_ptr->effects()->hallucination().is_hallucinated() && monster.is_original_ap();
}

/*!
 * @brief モンスターIDを取り、モンスター名をm_nameに代入する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @return std::string モンスター名
 */
std::string monster_name(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    return monster_desc(player_ptr, monster, 0x00);
}
