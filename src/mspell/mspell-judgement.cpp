/*!
 * @brief モンスター魔法の実装(対モンスター処理) / Monster spells (attack monster)
 * @date 2014/01/17
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "mspell/mspell-judgement.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "floor/line-of-sight.h"
#include "main/sound-definitions-table.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-status.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "spell/range-calc.h"
#include "system/angband-system.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"

/*!
 * @brief モンスターが敵対モンスターにビームを当てること可能かを判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monster 使用するモンスターへの参照
 * @param pos_target 目標座標
 * @return ビームが到達可能ならばTRUEを返す
 */
bool direct_beam(PlayerType *player_ptr, const MonsterEntity &monster, const Pos2D &pos_target)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto pos_source = monster.get_position();
    ProjectionPath grid_g(floor, AngbandSystem::get_instance().get_max_range(), player_ptr->get_position(), pos_source, pos_target, PROJECT_THRU);
    if (grid_g.path_num()) {
        return false;
    }

    const auto is_friend = monster.is_pet();
    auto hit2 = false;
    for (const auto &pos : grid_g) {
        const auto &grid = floor.get_grid(pos);
        if (pos == pos_target) {
            hit2 = true;
        } else if (is_friend && grid.has_monster() && !monster.is_hostile_to_melee(floor.m_list[grid.m_idx])) {
            return false;
        }

        if (is_friend && player_ptr->is_located_at(pos)) {
            return false;
        }
    }

    return hit2;
}

/*!
 * @brief モンスターが敵対モンスターに直接ブレスを当てることが可能かを判定する
 * @param pos_source 始点座標
 * @param pos_target 目標座標
 * @param rad 半径
 * @param typ 効果属性ID
 * @param is_friend TRUEならば、プレイヤーを巻き込む時にブレスの判定をFALSEにする。
 * @return ブレスを直接当てられるならばTRUEを返す
 */
bool breath_direct(PlayerType *player_ptr, const Pos2D &pos_source, const Pos2D &pos_target, int rad, AttributeType typ, bool is_friend)
{
    BIT_FLAGS flg;
    switch (typ) {
    case AttributeType::LITE:
    case AttributeType::LITE_WEAK:
        flg = PROJECT_LOS;
        break;
    case AttributeType::DISINTEGRATE:
        flg = PROJECT_DISI;
        break;
    default:
        flg = 0;
        break;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    ProjectionPath grid_g(floor, AngbandSystem::get_instance().get_max_range(), player_ptr->get_position(), pos_source, pos_target, flg);
    auto path_n = 0;
    Pos2D pos_breath = pos_source;
    for (const auto &pos : grid_g) {
        if (flg & PROJECT_DISI) {
            if (floor.can_block_disintegration_at(pos)) {
                break;
            }
        } else if (flg & PROJECT_LOS) {
            if (!floor.has_los_terrain_at(pos)) {
                break;
            }
        } else {
            if (!floor.has_terrain_characteristics(pos, TerrainCharacteristics::PROJECTION)) {
                break;
            }
        }

        pos_breath = pos;
        ++path_n;
    }

    auto hit2 = false;
    auto hityou = false;
    const auto p_pos = player_ptr->get_position();
    if (path_n == 0) {
        if (flg & PROJECT_DISI) {
            if (in_disintegration_range(floor, pos_source, pos_target) && (Grid::calc_distance(pos_source, pos_target) <= rad)) {
                hit2 = true;
            }
            if (in_disintegration_range(floor, pos_source, p_pos) && (Grid::calc_distance(pos_source, p_pos) <= rad)) {
                hityou = true;
            }
        } else if (flg & PROJECT_LOS) {
            if (los(floor, pos_source, pos_target) && (Grid::calc_distance(pos_source, pos_target) <= rad)) {
                hit2 = true;
            }
            if (los(floor, pos_source, p_pos) && (Grid::calc_distance(pos_source, p_pos) <= rad)) {
                hityou = true;
            }
        } else {
            if (projectable(floor, pos_source, pos_target) && (Grid::calc_distance(pos_source, pos_target) <= rad)) {
                hit2 = true;
            }
            if (projectable(floor, pos_source, p_pos) && (Grid::calc_distance(pos_source, p_pos) <= rad)) {
                hityou = true;
            }
        }
    } else {
        const auto positions = breath_shape(player_ptr, grid_g, path_n, rad, pos_source, pos_breath, typ);
        hit2 |= std::any_of(positions.begin(), positions.end(), [&pos_target](const auto &pair) { return pair.second == pos_target; });
        hityou |= std::any_of(positions.begin(), positions.end(), [&p_pos](const auto &pair) { return pair.second == p_pos; });
    }

    if (!hit2 || (is_friend && hityou)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターが特殊能力の目標地点を決める処理
 * @param floor フロアへの参照
 * @param p_pos プレイヤーの現在座標
 * @param pos_source 始点座標
 * @param pos_target_initial 目標座標の初期値
 * @param flg 判定のフラグ配列
 * @return 目標座標
 */
Pos2D get_project_point(const FloorType &floor, const Pos2D &p_pos, const Pos2D &pos_source, const Pos2D &pos_target_initial, BIT_FLAGS flags)
{
    ProjectionPath path_g(floor, AngbandSystem::get_instance().get_max_range(), p_pos, pos_source, pos_target_initial, flags);
    Pos2D pos_target = pos_source;
    for (const auto &pos : path_g) {
        if (!floor.has_terrain_characteristics(pos, TerrainCharacteristics::PROJECTION)) {
            break;
        }

        pos_target = pos;
    }

    return pos_target;
}

/*!
 * @brief モンスターが敵モンスターに魔力消去を使うかどうかを返す /
 * Check should monster cast dispel spell at other monster.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 術者のモンスターID
 * @param t_idx 目標のモンスターID
 * @return 魔力消去を使うべきならばTRUEを変えす。
 */
bool dispel_check_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
    const auto &t_ref = player_ptr->current_floor_ptr->m_list[t_idx];
    if (t_ref.is_invulnerable()) {
        return true;
    }

    constexpr auto threshold = 25;
    if ((t_ref.mspeed < (STANDARD_SPEED + threshold)) && t_ref.is_accelerated()) {
        return true;
    }

    if ((t_idx == player_ptr->riding) && dispel_check(player_ptr, m_idx)) {
        return true;
    }

    return false;
}

/*!
 * @brief モンスターがプレイヤーに魔力消去を与えるべきかを判定するルーチン
 * Check should monster cast dispel spell.
 * @param m_idx モンスターの構造体配列ID
 * @return 魔力消去をかけるべきならTRUEを返す。
 */
bool dispel_check(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    if (is_invuln(player_ptr)) {
        return true;
    }

    if (player_ptr->wraith_form) {
        return true;
    }

    if (player_ptr->shield) {
        return true;
    }

    if (player_ptr->magicdef) {
        return true;
    }

    if (player_ptr->multishadow) {
        return true;
    }

    if (player_ptr->dustrobe) {
        return true;
    }

    PlayerClass pc(player_ptr);
    if (player_ptr->shero && !pc.equals(PlayerClassType::BERSERKER)) {
        return true;
    }

    if (player_ptr->mimic_form == MimicKindType::DEMON_LORD) {
        return true;
    }

    const auto &floor_ref = *player_ptr->current_floor_ptr;
    const auto &monster = floor_ref.m_list[m_idx];
    const auto &monrace = monster.get_monrace();
    if (monrace.ability_flags.has(MonsterAbilityType::BR_ACID)) {
        if (!has_immune_acid(player_ptr) && (player_ptr->oppose_acid || music_singing(player_ptr, MUSIC_RESIST))) {
            return true;
        }

        if (player_ptr->special_defense & DEFENSE_ACID) {
            return true;
        }
    }

    if (monrace.ability_flags.has(MonsterAbilityType::BR_FIRE)) {
        if (!(PlayerRace(player_ptr).equals(PlayerRaceType::BALROG) && player_ptr->lev > 44)) {
            if (!has_immune_fire(player_ptr) && (player_ptr->oppose_fire || music_singing(player_ptr, MUSIC_RESIST))) {
                return true;
            }

            if (player_ptr->special_defense & DEFENSE_FIRE) {
                return true;
            }
        }
    }

    if (monrace.ability_flags.has(MonsterAbilityType::BR_ELEC)) {
        if (!has_immune_elec(player_ptr) && (player_ptr->oppose_elec || music_singing(player_ptr, MUSIC_RESIST))) {
            return true;
        }

        if (player_ptr->special_defense & DEFENSE_ELEC) {
            return true;
        }
    }

    if (monrace.ability_flags.has(MonsterAbilityType::BR_COLD)) {
        if (!has_immune_cold(player_ptr) && (player_ptr->oppose_cold || music_singing(player_ptr, MUSIC_RESIST))) {
            return true;
        }

        if (player_ptr->special_defense & DEFENSE_COLD) {
            return true;
        }
    }

    if (monrace.ability_flags.has_any_of({ MonsterAbilityType::BR_POIS, MonsterAbilityType::BR_NUKE }) && !(pc.equals(PlayerClassType::NINJA) && (player_ptr->lev > 44))) {
        if (player_ptr->oppose_pois || music_singing(player_ptr, MUSIC_RESIST)) {
            return true;
        }

        if (player_ptr->special_defense & DEFENSE_POIS) {
            return true;
        }
    }

    if (player_ptr->ult_res) {
        return true;
    }

    if (player_ptr->tsuyoshi) {
        return true;
    }

    if ((player_ptr->special_attack & ATTACK_ACID) && monrace.resistance_flags.has_none_of(RFR_EFF_IM_ACID_MASK)) {
        return true;
    }

    if ((player_ptr->special_attack & ATTACK_FIRE) && monrace.resistance_flags.has_none_of(RFR_EFF_IM_FIRE_MASK)) {
        return true;
    }

    if ((player_ptr->special_attack & ATTACK_ELEC) && monrace.resistance_flags.has_none_of(RFR_EFF_IM_ELEC_MASK)) {
        return true;
    }

    if ((player_ptr->special_attack & ATTACK_COLD) && monrace.resistance_flags.has_none_of(RFR_EFF_IM_COLD_MASK)) {
        return true;
    }

    if ((player_ptr->special_attack & ATTACK_POIS) && monrace.resistance_flags.has_none_of(RFR_EFF_IM_POISON_MASK)) {
        return true;
    }

    if ((player_ptr->pspeed < 145) && is_fast(player_ptr)) {
        return true;
    }

    constexpr auto threshold = 25;
    const auto threshold_speed = STANDARD_SPEED + threshold;
    if (player_ptr->lightspeed && (monster.mspeed <= threshold_speed)) {
        return true;
    }

    const auto &m_ref = floor_ref.m_list[player_ptr->riding];
    if (player_ptr->riding == 0) {
        return false;
    }

    return (floor_ref.m_list[player_ptr->riding].mspeed < threshold_speed) && m_ref.is_accelerated();
}
