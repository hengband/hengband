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
#include "dungeon/dungeon.h"
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
#include "player-info/race-info.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "spell/range-calc.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"

/*!
 * @brief モンスターが敵対モンスターにビームを当てること可能かを判定する /
 * Determine if a beam spell will hit the target.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y1 始点のY座標
 * @param x1 始点のX座標
 * @param y2 目標のY座標
 * @param x2 目標のX座標
 * @param m_ptr 使用するモンスターの構造体参照ポインタ
 * @return ビームが到達可能ならばTRUEを返す
 */
bool direct_beam(player_type *target_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, monster_type *m_ptr)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    uint16_t grid_g[512];
    int grid_n = projection_path(target_ptr, grid_g, get_max_range(target_ptr), y1, x1, y2, x2, PROJECT_THRU);
    if (!grid_n)
        return false;

    bool hit2 = false;
    POSITION y, x;
    bool is_friend = is_pet(m_ptr);
    for (int i = 0; i < grid_n; i++) {
        y = get_grid_y(grid_g[i]);
        x = get_grid_x(grid_g[i]);

        if (y == y2 && x == x2)
            hit2 = true;
        else if (is_friend && floor_ptr->grid_array[y][x].m_idx > 0 && !are_enemies(target_ptr, m_ptr, &floor_ptr->m_list[floor_ptr->grid_array[y][x].m_idx])) {
            return false;
        }

        if (is_friend && player_bold(target_ptr, y, x))
            return false;
    }

    if (!hit2)
        return false;
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
bool breath_direct(player_type *master_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, POSITION rad, EFFECT_ID typ, bool is_friend)
{
    BIT_FLAGS flg;
    switch (typ) {
    case GF_LITE:
    case GF_LITE_WEAK:
        flg = PROJECT_LOS;
        break;
    case GF_DISINTEGRATE:
        flg = PROJECT_DISI;
        break;
    default:
        flg = 0;
        break;
    }

    uint16_t grid_g[512];
    int grid_n = projection_path(master_ptr, grid_g, get_max_range(master_ptr), y1, x1, y2, x2, flg);
    int i;
    POSITION y = y1;
    POSITION x = x1;
    for (i = 0; i < grid_n; ++i) {
        int ny = get_grid_y(grid_g[i]);
        int nx = get_grid_x(grid_g[i]);

        if (flg & PROJECT_DISI) {
            if (cave_stop_disintegration(master_ptr->current_floor_ptr, ny, nx))
                break;
        } else if (flg & PROJECT_LOS) {
            if (!cave_los_bold(master_ptr->current_floor_ptr, ny, nx))
                break;
        } else {
            if (!cave_has_flag_bold(master_ptr->current_floor_ptr, ny, nx, FF::PROJECT))
                break;
        }

        y = ny;
        x = nx;
    }

    grid_n = i;
    bool hit2 = false;
    bool hityou = false;
    if (!grid_n) {
        if (flg & PROJECT_DISI) {
            if (in_disintegration_range(master_ptr->current_floor_ptr, y1, x1, y2, x2) && (distance(y1, x1, y2, x2) <= rad))
                hit2 = true;
            if (in_disintegration_range(master_ptr->current_floor_ptr, y1, x1, master_ptr->y, master_ptr->x)
                && (distance(y1, x1, master_ptr->y, master_ptr->x) <= rad))
                hityou = true;
        } else if (flg & PROJECT_LOS) {
            if (los(master_ptr, y1, x1, y2, x2) && (distance(y1, x1, y2, x2) <= rad))
                hit2 = true;
            if (los(master_ptr, y1, x1, master_ptr->y, master_ptr->x) && (distance(y1, x1, master_ptr->y, master_ptr->x) <= rad))
                hityou = true;
        } else {
            if (projectable(master_ptr, y1, x1, y2, x2) && (distance(y1, x1, y2, x2) <= rad))
                hit2 = true;
            if (projectable(master_ptr, y1, x1, master_ptr->y, master_ptr->x) && (distance(y1, x1, master_ptr->y, master_ptr->x) <= rad))
                hityou = true;
        }
    } else {
        int grids = 0;
        POSITION gx[1024], gy[1024];
        POSITION gm[32];
        POSITION gm_rad = rad;
        breath_shape(master_ptr, grid_g, grid_n, &grids, gx, gy, gm, &gm_rad, rad, y1, x1, y, x, typ);
        for (i = 0; i < grids; i++) {
            y = gy[i];
            x = gx[i];
            if ((y == y2) && (x == x2))
                hit2 = true;
            if (player_bold(master_ptr, y, x))
                hityou = true;
        }
    }

    if (!hit2)
        return false;
    if (is_friend && hityou)
        return false;

    return true;
}

/*!
 * @brief モンスターが特殊能力の目標地点を決める処理 /
 * Get the actual center point of ball spells (rad > 1) (originally from TOband)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param sy 始点のY座標
 * @param sx 始点のX座標
 * @param ty 目標Y座標を返す参照ポインタ
 * @param tx 目標X座標を返す参照ポインタ
 * @param flg 判定のフラグ配列
 */
void get_project_point(player_type *target_ptr, POSITION sy, POSITION sx, POSITION *ty, POSITION *tx, BIT_FLAGS flg)
{
    uint16_t path_g[128];
    int path_n = projection_path(target_ptr, path_g, get_max_range(target_ptr), sy, sx, *ty, *tx, flg);
    *ty = sy;
    *tx = sx;
    for (int i = 0; i < path_n; i++) {
        sy = get_grid_y(path_g[i]);
        sx = get_grid_x(path_g[i]);
        if (!cave_has_flag_bold(target_ptr->current_floor_ptr, sy, sx, FF::PROJECT))
            break;

        *ty = sy;
        *tx = sx;
    }
}

/*!
 * @brief モンスターが敵モンスターに魔力消去を使うかどうかを返す /
 * Check should monster cast dispel spell at other monster.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 術者のモンスターID
 * @param t_idx 目標のモンスターID
 * @return 魔力消去を使うべきならばTRUEを変えす。
 */
bool dispel_check_monster(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
    monster_type *t_ptr = &target_ptr->current_floor_ptr->m_list[t_idx];
    if (monster_invulner_remaining(t_ptr))
        return true;

    if ((t_ptr->mspeed < 135) && monster_fast_remaining(t_ptr))
        return true;

    if ((t_idx == target_ptr->riding) && dispel_check(target_ptr, m_idx))
        return true;

    return false;
}

/*!
 * @brief モンスターがプレイヤーに魔力消去を与えるべきかを判定するルーチン
 * Check should monster cast dispel spell.
 * @param m_idx モンスターの構造体配列ID
 * @return 魔力消去をかけるべきならTRUEを返す。
 */
bool dispel_check(player_type *creature_ptr, MONSTER_IDX m_idx)
{
    if (is_invuln(creature_ptr))
        return true;

    if (creature_ptr->wraith_form)
        return true;

    if (creature_ptr->shield)
        return true;

    if (creature_ptr->magicdef)
        return true;

    if (creature_ptr->multishadow)
        return true;

    if (creature_ptr->dustrobe)
        return true;

    if (creature_ptr->shero && (creature_ptr->pclass != CLASS_BERSERKER))
        return true;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD)
        return true;

    monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (r_ptr->ability_flags.has(RF_ABILITY::BR_ACID)) {
        if (!has_immune_acid(creature_ptr) && (creature_ptr->oppose_acid || music_singing(creature_ptr, MUSIC_RESIST)))
            return true;

        if (creature_ptr->special_defense & DEFENSE_ACID)
            return true;
    }

    if (r_ptr->ability_flags.has(RF_ABILITY::BR_FIRE)) {
        if (!((creature_ptr->prace == player_race_type::BALROG) && creature_ptr->lev > 44)) {
            if (!has_immune_fire(creature_ptr) && (creature_ptr->oppose_fire || music_singing(creature_ptr, MUSIC_RESIST)))
                return true;

            if (creature_ptr->special_defense & DEFENSE_FIRE)
                return true;
        }
    }

    if (r_ptr->ability_flags.has(RF_ABILITY::BR_ELEC)) {
        if (!has_immune_elec(creature_ptr) && (creature_ptr->oppose_elec || music_singing(creature_ptr, MUSIC_RESIST)))
            return true;

        if (creature_ptr->special_defense & DEFENSE_ELEC)
            return true;
    }

    if (r_ptr->ability_flags.has(RF_ABILITY::BR_COLD)) {
        if (!has_immune_cold(creature_ptr) && (creature_ptr->oppose_cold || music_singing(creature_ptr, MUSIC_RESIST)))
            return true;

        if (creature_ptr->special_defense & DEFENSE_COLD)
            return true;
    }

    if (r_ptr->ability_flags.has_any_of({ RF_ABILITY::BR_POIS, RF_ABILITY::BR_NUKE }) && !((creature_ptr->pclass == CLASS_NINJA) && (creature_ptr->lev > 44))) {
        if (creature_ptr->oppose_pois || music_singing(creature_ptr, MUSIC_RESIST))
            return true;

        if (creature_ptr->special_defense & DEFENSE_POIS)
            return true;
    }

    if (creature_ptr->ult_res)
        return true;

    if (creature_ptr->tsuyoshi)
        return true;

    if ((creature_ptr->special_attack & ATTACK_ACID) && !(r_ptr->flagsr & RFR_EFF_IM_ACID_MASK))
        return true;

    if ((creature_ptr->special_attack & ATTACK_FIRE) && !(r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK))
        return true;

    if ((creature_ptr->special_attack & ATTACK_ELEC) && !(r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK))
        return true;

    if ((creature_ptr->special_attack & ATTACK_COLD) && !(r_ptr->flagsr & RFR_EFF_IM_COLD_MASK))
        return true;

    if ((creature_ptr->special_attack & ATTACK_POIS) && !(r_ptr->flagsr & RFR_EFF_IM_POIS_MASK))
        return true;

    if ((creature_ptr->pspeed < 145) && is_fast(creature_ptr))
        return true;

    if (creature_ptr->lightspeed && (m_ptr->mspeed < 136))
        return true;

    if (creature_ptr->riding && (creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].mspeed < 135)
        && monster_fast_remaining(&creature_ptr->current_floor_ptr->m_list[creature_ptr->riding]))
        return true;

    return false;
}
