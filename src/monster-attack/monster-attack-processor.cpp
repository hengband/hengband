/*!
 * @brief モンスターの攻撃に関する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-attack/monster-attack-processor.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/cave.h"
#include "melee/monster-attack-monster.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief モンスターが移動した結果、そこにプレイヤーがいたら直接攻撃を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param ny 移動後の、モンスターのY座標
 * @param nx 移動後の、モンスターのX座標
 * @details
 * 反攻撃の洞窟など、直接攻撃ができない場所では処理をスキップする
 */
void exe_monster_attack_to_player(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION ny, POSITION nx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto *m_ptr = &floor.m_list[m_idx];
    auto *r_ptr = &m_ptr->get_monrace();
    if (!turn_flags_ptr->do_move || !player_ptr->is_located_at({ ny, nx })) {
        return;
    }

    if (r_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW)) {
        if (is_original_ap_and_seen(player_ptr, m_ptr)) {
            r_ptr->r_behavior_flags.set(MonsterBehaviorType::NEVER_BLOW);
        }

        turn_flags_ptr->do_move = false;
    }

    if (turn_flags_ptr->do_move && floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE) && !m_ptr->is_confused()) {
        if (r_ptr->behavior_flags.has_not(MonsterBehaviorType::STUPID)) {
            turn_flags_ptr->do_move = false;
        } else if (is_original_ap_and_seen(player_ptr, m_ptr)) {
            r_ptr->r_behavior_flags.set(MonsterBehaviorType::STUPID);
        }
    }

    if (!turn_flags_ptr->do_move) {
        return;
    }

    if (!player_ptr->riding || one_in_(2)) {
        MonsterAttackPlayer(player_ptr, m_idx).make_attack_normal();
        turn_flags_ptr->do_move = false;
        turn_flags_ptr->do_turn = true;
    }
}

/*!
 * @brief モンスターからモンスターへの直接攻撃を実行する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param g_ptr グリッドへの参照ポインタ
 */
static bool exe_monster_attack_to_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, Grid *g_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto *m_ptr = &floor.m_list[m_idx];
    auto *r_ptr = &m_ptr->get_monrace();
    MonsterEntity *y_ptr;
    y_ptr = &player_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if ((r_ptr->behavior_flags.has_not(MonsterBehaviorType::KILL_BODY)) && is_original_ap_and_seen(player_ptr, m_ptr)) {
        r_ptr->r_behavior_flags.set(MonsterBehaviorType::KILL_BODY);
    }

    if (!MonsterRace(y_ptr->r_idx).is_valid() || (y_ptr->hp < 0)) {
        return false;
    }
    if (monst_attack_monst(player_ptr, m_idx, g_ptr->m_idx)) {
        return true;
    }
    if (floor.get_dungeon_definition().flags.has_not(DungeonFeatureType::NO_MELEE)) {
        return false;
    }
    if (m_ptr->is_confused()) {
        return true;
    }
    if (r_ptr->behavior_flags.has_not(MonsterBehaviorType::STUPID)) {
        return false;
    }

    if (is_original_ap_and_seen(player_ptr, m_ptr)) {
        r_ptr->r_behavior_flags.set(MonsterBehaviorType::STUPID);
    }

    return true;
}

/*!
 * @brief モンスターからモンスターへの攻撃処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param g_ptr グリッドへの参照ポインタ
 * @param can_cross モンスターが地形を踏破できるならばTRUE
 * @return ターン消費が発生したらTRUE
 */
bool process_monster_attack_to_monster(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, Grid *g_ptr, bool can_cross)
{
    if (!turn_flags_ptr->do_move || (g_ptr->m_idx == 0)) {
        return false;
    }

    turn_flags_ptr->do_move = false;
    const auto &monster_from = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto &monrace_from = monster_from.get_monrace();
    const auto &monster_to = player_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
    const auto &monrace_to = monster_to.get_monrace();
    auto do_kill_body = monrace_from.behavior_flags.has(MonsterBehaviorType::KILL_BODY) && monrace_from.behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW);
    do_kill_body &= (monrace_from.mexp * monrace_from.level > monrace_to.mexp * monrace_to.level);
    do_kill_body &= (g_ptr->m_idx != player_ptr->riding);
    if (do_kill_body || monster_from.is_hostile_to_melee(monster_to) || monster_from.is_confused()) {
        return exe_monster_attack_to_monster(player_ptr, m_idx, g_ptr);
    }

    auto do_move_body = monrace_from.behavior_flags.has(MonsterBehaviorType::MOVE_BODY) && monrace_from.behavior_flags.has_not(MonsterBehaviorType::NEVER_MOVE);
    do_move_body &= (monrace_from.mexp > monrace_to.mexp);
    do_move_body &= can_cross;
    do_move_body &= (g_ptr->m_idx != player_ptr->riding);
    do_move_body &= monster_can_cross_terrain(player_ptr, player_ptr->current_floor_ptr->grid_array[monster_from.fy][monster_from.fx].feat, &monrace_to, 0);
    if (do_move_body) {
        turn_flags_ptr->do_move = true;
        turn_flags_ptr->did_move_body = true;
        (void)set_monster_csleep(player_ptr, g_ptr->m_idx, 0);
    }

    return false;
}
