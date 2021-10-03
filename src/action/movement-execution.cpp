/*!
 * @file movement-execution.cpp
 * @brief プレイヤーの歩行勝利実行定義
 */

#include "action/movement-execution.h"
#include "action/open-close-execution.h"
#include "artifact/fixed-art-types.h"
#include "cmd-action/cmd-attack.h"
#include "core/disturbance.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "floor/pattern-walk.h"
#include "floor/geometry.h"
#include "game-option/input-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "mutation/mutation-flag-types.h"
#include "object/warning.h"
#include "player-status/player-energy.h"
#include "player/player-move.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "system/object-type-definition.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#ifdef JP
#else
#include "locale/english.h"
#endif

/*!
 * Determine if a "boundary" grid is "floor mimic"
 * @param grid_type *g_ptr
 * @param feature_type *f_ptr
 * @param feature_type  *mimic_f_ptr
 * @return 移動不能であればTRUE
 * @todo 負論理なので反転させたい
 */
static bool boundary_floor(grid_type *g_ptr, feature_type *f_ptr, feature_type *mimic_f_ptr)
{
    bool is_boundary_floor = g_ptr->mimic > 0;
    is_boundary_floor &= permanent_wall(f_ptr);
    is_boundary_floor &= mimic_f_ptr->flags.has_any_of({ FF::MOVE, FF::CAN_FLY });
    is_boundary_floor &= mimic_f_ptr->flags.has(FF::PROJECT);
    is_boundary_floor &= mimic_f_ptr->flags.has_not(FF::OPEN);
    return is_boundary_floor;
}

/*!
 * @brief 該当地形のトラップがプレイヤーにとって無効かどうかを判定して返す /
 * Move player in the given direction, with the given "pickup" flag.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 移動方向ID
 * @param do_pickup 罠解除を試みながらの移動ならばTRUE
 * @param break_trap トラップ粉砕処理を行うならばTRUE
 * @return 実際に移動が行われたならばTRUEを返す。
 * @note
 * This routine should (probably) always induce energy expenditure.\n
 * @details
 * Note that moving will *always* take a turn, and will *always* hit\n
 * any monster which might be in the destination grid.  Previously,\n
 * moving into walls was "free" and did NOT hit invisible monsters.\n
 */
void exe_movement(player_type *player_ptr, DIRECTION dir, bool do_pickup, bool break_trap)
{
    POSITION y = player_ptr->y + ddy[dir];
    POSITION x = player_ptr->x + ddx[dir];
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    bool p_can_enter = player_can_enter(player_ptr, g_ptr->feat, CEM_P_CAN_ENTER_PATTERN);
    bool stormbringer = false;
    if (!floor_ptr->dun_level && !player_ptr->wild_mode && ((x == 0) || (x == MAX_WID - 1) || (y == 0) || (y == MAX_HGT - 1))) {
        if (g_ptr->mimic && player_can_enter(player_ptr, g_ptr->mimic, 0)) {
            if ((y == 0) && (x == 0)) {
                player_ptr->wilderness_y--;
                player_ptr->wilderness_x--;
                player_ptr->oldpy = floor_ptr->height - 2;
                player_ptr->oldpx = floor_ptr->width - 2;
                player_ptr->ambush_flag = false;
            } else if ((y == 0) && (x == MAX_WID - 1)) {
                player_ptr->wilderness_y--;
                player_ptr->wilderness_x++;
                player_ptr->oldpy = floor_ptr->height - 2;
                player_ptr->oldpx = 1;
                player_ptr->ambush_flag = false;
            } else if ((y == MAX_HGT - 1) && (x == 0)) {
                player_ptr->wilderness_y++;
                player_ptr->wilderness_x--;
                player_ptr->oldpy = 1;
                player_ptr->oldpx = floor_ptr->width - 2;
                player_ptr->ambush_flag = false;
            } else if ((y == MAX_HGT - 1) && (x == MAX_WID - 1)) {
                player_ptr->wilderness_y++;
                player_ptr->wilderness_x++;
                player_ptr->oldpy = 1;
                player_ptr->oldpx = 1;
                player_ptr->ambush_flag = false;
            } else if (y == 0) {
                player_ptr->wilderness_y--;
                player_ptr->oldpy = floor_ptr->height - 2;
                player_ptr->oldpx = x;
                player_ptr->ambush_flag = false;
            } else if (y == MAX_HGT - 1) {
                player_ptr->wilderness_y++;
                player_ptr->oldpy = 1;
                player_ptr->oldpx = x;
                player_ptr->ambush_flag = false;
            } else if (x == 0) {
                player_ptr->wilderness_x--;
                player_ptr->oldpx = floor_ptr->width - 2;
                player_ptr->oldpy = y;
                player_ptr->ambush_flag = false;
            } else if (x == MAX_WID - 1) {
                player_ptr->wilderness_x++;
                player_ptr->oldpx = 1;
                player_ptr->oldpy = y;
                player_ptr->ambush_flag = false;
            }

            player_ptr->leaving = true;
            PlayerEnergy(player_ptr).set_player_turn_energy(100);
            return;
        }

        p_can_enter = false;
    }

    monster_type *m_ptr;
    m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
    if (player_ptr->inventory_list[INVEN_MAIN_HAND].name1 == ART_STORMBRINGER)
        stormbringer = true;

    if (player_ptr->inventory_list[INVEN_SUB_HAND].name1 == ART_STORMBRINGER)
        stormbringer = true;

    feature_type *f_ptr = &f_info[g_ptr->feat];
    bool p_can_kill_walls = has_kill_wall(player_ptr) && f_ptr->flags.has(FF::HURT_DISI) && (!p_can_enter || f_ptr->flags.has_not(FF::LOS))
        && f_ptr->flags.has_not(FF::PERMANENT);
    GAME_TEXT m_name[MAX_NLEN];
    bool can_move = true;
    bool do_past = false;
    if (g_ptr->m_idx && (m_ptr->ml || p_can_enter || p_can_kill_walls)) {
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        auto effects = player_ptr->effects();
        auto is_stunned = effects->stun()->is_stunned();
        if (!is_hostile(m_ptr)
            && !(player_ptr->confused || player_ptr->hallucinated || !m_ptr->ml || is_stunned
                || (player_ptr->muta.has(MUTA::BERS_RAGE) && is_shero(player_ptr)))
            && pattern_seq(player_ptr, player_ptr->y, player_ptr->x, y, x) && (p_can_enter || p_can_kill_walls)) {
            (void)set_monster_csleep(player_ptr, g_ptr->m_idx, 0);
            monster_desc(player_ptr, m_name, m_ptr, 0);
            if (m_ptr->ml) {
                if (!player_ptr->hallucinated)
                    monster_race_track(player_ptr, m_ptr->ap_r_idx);

                health_track(player_ptr, g_ptr->m_idx);
            }

            if ((stormbringer && (randint1(1000) > 666)) || (player_ptr->pclass == CLASS_BERSERKER)) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
                can_move = false;
            } else if (monster_can_cross_terrain(player_ptr, floor_ptr->grid_array[player_ptr->y][player_ptr->x].feat, r_ptr, 0)) {
                do_past = true;
            } else {
                msg_format(_("%^sが邪魔だ！", "%^s is in your way!"), m_name);
                PlayerEnergy(player_ptr).reset_player_turn();
                can_move = false;
            }
        } else {
            do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
            can_move = false;
        }
    }

    monster_type *riding_m_ptr = &floor_ptr->m_list[player_ptr->riding];
    monster_race *riding_r_ptr = &r_info[player_ptr->riding ? riding_m_ptr->r_idx : 0];
    PlayerEnergy energy(player_ptr);
    if (can_move && player_ptr->riding) {
        if (riding_r_ptr->flags1 & RF1_NEVER_MOVE) {
            msg_print(_("動けない！", "Can't move!"));
            energy.reset_player_turn();
            can_move = false;
            disturb(player_ptr, false, true);
        } else if (monster_fear_remaining(riding_m_ptr)) {
            GAME_TEXT steed_name[MAX_NLEN];
            monster_desc(player_ptr, steed_name, riding_m_ptr, 0);
            msg_format(_("%sが恐怖していて制御できない。", "%^s is too scared to control."), steed_name);
            can_move = false;
            disturb(player_ptr, false, true);
        } else if (player_ptr->riding_ryoute) {
            can_move = false;
            disturb(player_ptr, false, true);
        } else if (f_ptr->flags.has(FF::CAN_FLY) && (riding_r_ptr->flags7 & RF7_CAN_FLY)) {
            /* Allow moving */
        } else if (f_ptr->flags.has(FF::CAN_SWIM) && (riding_r_ptr->flags7 & RF7_CAN_SWIM)) {
            /* Allow moving */
        } else if (f_ptr->flags.has(FF::WATER) && !(riding_r_ptr->flags7 & RF7_AQUATIC)
            && (f_ptr->flags.has(FF::DEEP) || (riding_r_ptr->flags2 & RF2_AURA_FIRE))) {
            msg_format(_("%sの上に行けない。", "Can't swim."), f_info[g_ptr->get_feat_mimic()].name.c_str());
            energy.reset_player_turn();
            can_move = false;
            disturb(player_ptr, false, true);
        } else if (f_ptr->flags.has_not(FF::WATER) && (riding_r_ptr->flags7 & RF7_AQUATIC)) {
            msg_format(_("%sから上がれない。", "Can't land."), f_info[floor_ptr->grid_array[player_ptr->y][player_ptr->x].get_feat_mimic()].name.c_str());
            energy.reset_player_turn();
            can_move = false;
            disturb(player_ptr, false, true);
        } else if (f_ptr->flags.has(FF::LAVA) && !(riding_r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK)) {
            msg_format(_("%sの上に行けない。", "Too hot to go through."), f_info[g_ptr->get_feat_mimic()].name.c_str());
            energy.reset_player_turn();
            can_move = false;
            disturb(player_ptr, false, true);
        }

        if (can_move && monster_stunned_remaining(riding_m_ptr) && one_in_(2)) {
            GAME_TEXT steed_name[MAX_NLEN];
            monster_desc(player_ptr, steed_name, riding_m_ptr, 0);
            msg_format(_("%sが朦朧としていてうまく動けない！", "You cannot control stunned %s!"), steed_name);
            can_move = false;
            disturb(player_ptr, false, true);
        }
    }

    if (!can_move) {
    } else if (f_ptr->flags.has_not(FF::MOVE) && f_ptr->flags.has(FF::CAN_FLY) && !player_ptr->levitation) {
        msg_format(_("空を飛ばないと%sの上には行けない。", "You need to fly to go through the %s."), f_info[g_ptr->get_feat_mimic()].name.c_str());
        energy.reset_player_turn();
        player_ptr->running = 0;
        can_move = false;
    } else if (f_ptr->flags.has(FF::TREE) && !p_can_kill_walls) {
        if ((player_ptr->pclass != CLASS_RANGER) && !player_ptr->levitation && (!player_ptr->riding || !(riding_r_ptr->flags8 & RF8_WILD_WOOD))) {
            energy.mul_player_turn_energy(2);
        }
    } else if ((do_pickup != easy_disarm) && f_ptr->flags.has(FF::DISARM) && !g_ptr->mimic) {
        if (!trap_can_be_ignored(player_ptr, g_ptr->feat)) {
            (void)exe_disarm(player_ptr, y, x, dir);
            return;
        }
    } else if (!p_can_enter && !p_can_kill_walls) {
        FEAT_IDX feat = g_ptr->get_feat_mimic();
        feature_type *mimic_f_ptr = &f_info[feat];
        concptr name = mimic_f_ptr->name.c_str();
        can_move = false;
        if (!g_ptr->is_mark() && !player_can_see_bold(player_ptr, y, x)) {
            if (boundary_floor(g_ptr, f_ptr, mimic_f_ptr))
                msg_print(_("それ以上先には進めないようだ。", "You feel you cannot go any more."));
            else {
#ifdef JP
                msg_format("%sが行く手をはばんでいるようだ。", name);
#else
                msg_format("You feel %s %s blocking your way.", is_a_vowel(name[0]) ? "an" : "a", name);
#endif
                g_ptr->info |= (CAVE_MARK);
                lite_spot(player_ptr, y, x);
            }
        } else {
            auto effects = player_ptr->effects();
            auto is_stunned = effects->stun()->is_stunned();
            if (boundary_floor(g_ptr, f_ptr, mimic_f_ptr)) {
                msg_print(_("それ以上先には進めない。", "You cannot go any more."));
                if (!(player_ptr->confused || is_stunned || player_ptr->hallucinated)) {
                    energy.reset_player_turn();
                }
            } else {
                if (easy_open && is_closed_door(player_ptr, feat) && easy_open_door(player_ptr, y, x))
                    return;

#ifdef JP
                msg_format("%sが行く手をはばんでいる。", name);
#else
                msg_format("There is %s %s blocking your way.", is_a_vowel(name[0]) ? "an" : "a", name);
#endif
                if (!(player_ptr->confused || is_stunned || player_ptr->hallucinated))
                    energy.reset_player_turn();
            }
        }

        disturb(player_ptr, false, true);
        if (!boundary_floor(g_ptr, f_ptr, mimic_f_ptr))
            sound(SOUND_HITWALL);
    }

    if (can_move && !pattern_seq(player_ptr, player_ptr->y, player_ptr->x, y, x)) {
        auto effects = player_ptr->effects();
        auto is_stunned = effects->stun()->is_stunned();
        if (!(player_ptr->confused || is_stunned || player_ptr->hallucinated)) {
            energy.reset_player_turn();
        }

        disturb(player_ptr, false, true);
        can_move = false;
    }

    if (!can_move)
        return;

    if (player_ptr->warning && (!process_warning(player_ptr, x, y))) {
        energy.set_player_turn_energy(25);
        return;
    }

    if (do_past)
        msg_format(_("%sを押し退けた。", "You push past %s."), m_name);

    if (player_ptr->wild_mode) {
        if (ddy[dir] > 0)
            player_ptr->oldpy = 1;

        if (ddy[dir] < 0)
            player_ptr->oldpy = MAX_HGT - 2;

        if (ddy[dir] == 0)
            player_ptr->oldpy = MAX_HGT / 2;

        if (ddx[dir] > 0)
            player_ptr->oldpx = 1;

        if (ddx[dir] < 0)
            player_ptr->oldpx = MAX_WID - 2;

        if (ddx[dir] == 0)
            player_ptr->oldpx = MAX_WID / 2;
    }

    if (p_can_kill_walls) {
        cave_alter_feat(player_ptr, y, x, FF::HURT_DISI);
        player_ptr->update |= PU_FLOW;
    }

    uint32_t mpe_mode = MPE_ENERGY_USE;
    if (do_pickup != always_pickup)
        mpe_mode |= MPE_DO_PICKUP;

    if (break_trap)
        mpe_mode |= MPE_BREAK_TRAP;

    (void)move_player_effect(player_ptr, y, x, mpe_mode);
}
