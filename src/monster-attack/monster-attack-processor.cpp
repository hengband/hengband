/*!
 * @brief モンスターの攻撃に関する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-attack/monster-attack-processor.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "melee/monster-attack-monster.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "system/floor-type-definition.h"

/*!
 * @brief モンスターが移動した結果、そこにプレーヤーがいたら直接攻撃を行う
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param ny 移動後の、モンスターのY座標
 * @param nx 移動後の、モンスターのX座標
 * @return なし
 * @details
 * 反攻撃の洞窟など、直接攻撃ができない場所では処理をスキップする
 */
void exe_monster_attack_to_player(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION ny, POSITION nx)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (!turn_flags_ptr->do_move || !player_bold(target_ptr, ny, nx))
        return;

    if (r_ptr->flags1 & RF1_NEVER_BLOW) {
        if (is_original_ap_and_seen(target_ptr, m_ptr))
            r_ptr->r_flags1 |= (RF1_NEVER_BLOW);

        turn_flags_ptr->do_move = FALSE;
    }

    if (turn_flags_ptr->do_move && ((d_info[target_ptr->dungeon_idx].flags1 & DF1_NO_MELEE) != 0) && !monster_confused_remaining(m_ptr)) {
        if (!(r_ptr->flags2 & RF2_STUPID))
            turn_flags_ptr->do_move = FALSE;
        else if (is_original_ap_and_seen(target_ptr, m_ptr))
            r_ptr->r_flags2 |= (RF2_STUPID);
    }

    if (!turn_flags_ptr->do_move)
        return;

    if (!target_ptr->riding || one_in_(2)) {
        (void)make_attack_normal(target_ptr, m_idx);
        turn_flags_ptr->do_move = FALSE;
        turn_flags_ptr->do_turn = TRUE;
    }
}

/*!
 * @brief モンスターからモンスターへの直接攻撃を実行する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param g_ptr グリッドへの参照ポインタ
 */
static bool exe_monster_attack_to_monster(player_type *target_ptr, MONSTER_IDX m_idx, grid_type *g_ptr)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    monster_type *y_ptr;
    y_ptr = &target_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
    if ((r_ptr->flags1 & RF1_NEVER_BLOW) != 0)
        return FALSE;

    if (((r_ptr->flags2 & RF2_KILL_BODY) == 0) && is_original_ap_and_seen(target_ptr, m_ptr))
        r_ptr->r_flags2 |= (RF2_KILL_BODY);

    if ((y_ptr->r_idx == 0) || (y_ptr->hp < 0))
        return FALSE;
    if (monst_attack_monst(target_ptr, m_idx, g_ptr->m_idx))
        return TRUE;
    if ((d_info[target_ptr->dungeon_idx].flags1 & DF1_NO_MELEE) == 0)
        return FALSE;
    if (monster_confused_remaining(m_ptr))
        return TRUE;
    if ((r_ptr->flags2 & RF2_STUPID) == 0)
        return FALSE;

    if (is_original_ap_and_seen(target_ptr, m_ptr))
        r_ptr->r_flags2 |= (RF2_STUPID);

    return TRUE;
}

/*!
 * @brief モンスターからモンスターへの攻撃処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param g_ptr グリッドへの参照ポインタ
 * @param can_cross モンスターが地形を踏破できるならばTRUE
 * @return ターン消費が発生したらTRUE
 */
bool process_monster_attack_to_monster(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, grid_type *g_ptr, bool can_cross)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    monster_type *y_ptr;
    y_ptr = &target_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
    if (!turn_flags_ptr->do_move || (g_ptr->m_idx == 0))
        return FALSE;

    monster_race *z_ptr = &r_info[y_ptr->r_idx];
    turn_flags_ptr->do_move = FALSE;
    if ((((r_ptr->flags2 & RF2_KILL_BODY) != 0) && ((r_ptr->flags1 & RF1_NEVER_BLOW) == 0) && (r_ptr->mexp * r_ptr->level > z_ptr->mexp * z_ptr->level)
            && can_cross && (g_ptr->m_idx != target_ptr->riding))
        || are_enemies(target_ptr, m_ptr, y_ptr) || monster_confused_remaining(m_ptr)) {
        return exe_monster_attack_to_monster(target_ptr, m_idx, g_ptr);
    }

    if (((r_ptr->flags2 & RF2_MOVE_BODY) != 0) && ((r_ptr->flags1 & RF1_NEVER_MOVE) == 0) && (r_ptr->mexp > z_ptr->mexp) && can_cross
        && (g_ptr->m_idx != target_ptr->riding)
        && monster_can_cross_terrain(target_ptr, target_ptr->current_floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].feat, z_ptr, 0)) {
        turn_flags_ptr->do_move = TRUE;
        turn_flags_ptr->did_move_body = TRUE;
        (void)set_monster_csleep(target_ptr, g_ptr->m_idx, 0);
    }

    return FALSE;
}
