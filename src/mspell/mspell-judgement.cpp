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
#include "floor/cave.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "grid/feature-flag-types.h"
#include "main/sound-definitions-table.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-info.h"
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
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"

/*!
 * @brief モンスターが敵対モンスターにビームを当てること可能かを判定する /
 * Determine if a beam spell will hit the target.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y1 始点のY座標
 * @param x1 始点のX座標
 * @param y2 目標のY座標
 * @param x2 目標のX座標
 * @param m_ptr 使用するモンスターの構造体参照ポインタ
 * @return ビームが到達可能ならばTRUEを返す
 */
bool direct_beam(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, monster_type *m_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    projection_path grid_g(player_ptr, get_max_range(player_ptr), y1, x1, y2, x2, PROJECT_THRU);
    if (grid_g.path_num()) {
        return false;
    }

    bool hit2 = false;
    bool is_friend = m_ptr->is_pet();
    for (const auto &[y, x] : grid_g) {
        const auto &g_ref = floor_ptr->grid_array[y][x];
        if (y == y2 && x == x2) {
            hit2 = true;
        } else if (is_friend && g_ref.m_idx > 0 && !are_enemies(player_ptr, *m_ptr, floor_ptr->m_list[g_ref.m_idx])) {
            return false;
        }

        if (is_friend && player_bold(player_ptr, y, x)) {
            return false;
        }
    }

    if (!hit2) {
        return false;
    }
    return true;
}

/*!
 * @brief モンスターが敵対モンスターに直接ブレスを当てることが可能かを判定する /
 * Determine if a breath will hit the target.
 * @param y1 始点のY座標
 * @param x1 始点のX座標
 * @param y2 目標のY座標
 * @param x2 目標のX座標
 * @param rad 半径
 * @param typ 効果属性ID
 * @param is_friend TRUEならば、プレイヤーを巻き込む時にブレスの判定をFALSEにする。
 * @return ブレスを直接当てられるならばTRUEを返す
 */
bool breath_direct(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, POSITION rad, AttributeType typ, bool is_friend)
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

    projection_path grid_g(player_ptr, get_max_range(player_ptr), y1, x1, y2, x2, flg);
    int i = 0;
    POSITION y = y1;
    POSITION x = x1;
    for (const auto &[ny, nx] : grid_g) {
        if (flg & PROJECT_DISI) {
            if (cave_stop_disintegration(player_ptr->current_floor_ptr, ny, nx)) {
                break;
            }
        } else if (flg & PROJECT_LOS) {
            if (!cave_los_bold(player_ptr->current_floor_ptr, ny, nx)) {
                break;
            }
        } else {
            if (!cave_has_flag_bold(player_ptr->current_floor_ptr, ny, nx, TerrainCharacteristics::PROJECT)) {
                break;
            }
        }

        y = ny;
        x = nx;
        i++;
    }

    bool hit2 = false;
    bool hityou = false;
    if (i == 0) {
        if (flg & PROJECT_DISI) {
            if (in_disintegration_range(player_ptr->current_floor_ptr, y1, x1, y2, x2) && (distance(y1, x1, y2, x2) <= rad)) {
                hit2 = true;
            }
            if (in_disintegration_range(player_ptr->current_floor_ptr, y1, x1, player_ptr->y, player_ptr->x) && (distance(y1, x1, player_ptr->y, player_ptr->x) <= rad)) {
                hityou = true;
            }
        } else if (flg & PROJECT_LOS) {
            if (los(player_ptr, y1, x1, y2, x2) && (distance(y1, x1, y2, x2) <= rad)) {
                hit2 = true;
            }
            if (los(player_ptr, y1, x1, player_ptr->y, player_ptr->x) && (distance(y1, x1, player_ptr->y, player_ptr->x) <= rad)) {
                hityou = true;
            }
        } else {
            if (projectable(player_ptr, y1, x1, y2, x2) && (distance(y1, x1, y2, x2) <= rad)) {
                hit2 = true;
            }
            if (projectable(player_ptr, y1, x1, player_ptr->y, player_ptr->x) && (distance(y1, x1, player_ptr->y, player_ptr->x) <= rad)) {
                hityou = true;
            }
        }
    } else {
        int grids = 0;
        POSITION gx[1024], gy[1024];
        POSITION gm[32];
        POSITION gm_rad = rad;
        breath_shape(player_ptr, grid_g, grid_g.path_num(), &grids, gx, gy, gm, &gm_rad, rad, y1, x1, y, x, typ);
        for (i = 0; i < grids; i++) {
            y = gy[i];
            x = gx[i];
            if ((y == y2) && (x == x2)) {
                hit2 = true;
            }
            if (player_bold(player_ptr, y, x)) {
                hityou = true;
            }
        }
    }

    if (!hit2) {
        return false;
    }
    if (is_friend && hityou) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターが特殊能力の目標地点を決める処理 /
 * Get the actual center point of ball spells (rad > 1) (originally from TOband)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sy 始点のY座標
 * @param sx 始点のX座標
 * @param ty 目標Y座標を返す参照ポインタ
 * @param tx 目標X座標を返す参照ポインタ
 * @param flg 判定のフラグ配列
 */
void get_project_point(PlayerType *player_ptr, POSITION sy, POSITION sx, POSITION *ty, POSITION *tx, BIT_FLAGS flg)
{
    projection_path path_g(player_ptr, get_max_range(player_ptr), sy, sx, *ty, *tx, flg);
    *ty = sy;
    *tx = sx;
    for (const auto &[y, x] : path_g) {
        if (!cave_has_flag_bold(player_ptr->current_floor_ptr, y, x, TerrainCharacteristics::PROJECT)) {
            break;
        }

        *ty = y;
        *tx = x;
    }
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
    auto *m_ptr = &floor_ref.m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    if (r_ptr->ability_flags.has(MonsterAbilityType::BR_ACID)) {
        if (!has_immune_acid(player_ptr) && (player_ptr->oppose_acid || music_singing(player_ptr, MUSIC_RESIST))) {
            return true;
        }

        if (player_ptr->special_defense & DEFENSE_ACID) {
            return true;
        }
    }

    if (r_ptr->ability_flags.has(MonsterAbilityType::BR_FIRE)) {
        if (!(PlayerRace(player_ptr).equals(PlayerRaceType::BALROG) && player_ptr->lev > 44)) {
            if (!has_immune_fire(player_ptr) && (player_ptr->oppose_fire || music_singing(player_ptr, MUSIC_RESIST))) {
                return true;
            }

            if (player_ptr->special_defense & DEFENSE_FIRE) {
                return true;
            }
        }
    }

    if (r_ptr->ability_flags.has(MonsterAbilityType::BR_ELEC)) {
        if (!has_immune_elec(player_ptr) && (player_ptr->oppose_elec || music_singing(player_ptr, MUSIC_RESIST))) {
            return true;
        }

        if (player_ptr->special_defense & DEFENSE_ELEC) {
            return true;
        }
    }

    if (r_ptr->ability_flags.has(MonsterAbilityType::BR_COLD)) {
        if (!has_immune_cold(player_ptr) && (player_ptr->oppose_cold || music_singing(player_ptr, MUSIC_RESIST))) {
            return true;
        }

        if (player_ptr->special_defense & DEFENSE_COLD) {
            return true;
        }
    }

    if (r_ptr->ability_flags.has_any_of({ MonsterAbilityType::BR_POIS, MonsterAbilityType::BR_NUKE }) && !(pc.equals(PlayerClassType::NINJA) && (player_ptr->lev > 44))) {
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

    if ((player_ptr->special_attack & ATTACK_ACID) && r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_ACID_MASK)) {
        return true;
    }

    if ((player_ptr->special_attack & ATTACK_FIRE) && r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_FIRE_MASK)) {
        return true;
    }

    if ((player_ptr->special_attack & ATTACK_ELEC) && r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_ELEC_MASK)) {
        return true;
    }

    if ((player_ptr->special_attack & ATTACK_COLD) && r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_COLD_MASK)) {
        return true;
    }

    if ((player_ptr->special_attack & ATTACK_POIS) && r_ptr->resistance_flags.has_none_of(RFR_EFF_IM_POISON_MASK)) {
        return true;
    }

    if ((player_ptr->pspeed < 145) && is_fast(player_ptr)) {
        return true;
    }

    constexpr auto threshold = 25;
    const auto threshold_speed = STANDARD_SPEED + threshold;
    if (player_ptr->lightspeed && (m_ptr->mspeed <= threshold_speed)) {
        return true;
    }

    const auto &m_ref = floor_ref.m_list[player_ptr->riding];
    if (player_ptr->riding == 0) {
        return false;
    }

    return (floor_ref.m_list[player_ptr->riding].mspeed < threshold_speed) && m_ref.is_accelerated();
}
