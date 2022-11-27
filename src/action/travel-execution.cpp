/*!
 * @file travel-execution.cpp
 * @brief トラベル移動処理実装
 */

#include "action/travel-execution.h"
#include "action/movement-execution.h"
#include "action/run-execution.h"
#include "core/disturbance.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "player-status/player-energy.h"
#include "player/player-move.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

travel_type travel;

/*!
 * @brief トラベル機能の判定処理 /
 * Test for traveling
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param prev_dir 前回移動を行った元の方角ID
 * @return 次の方向
 */
static DIRECTION travel_test(PlayerType *player_ptr, DIRECTION prev_dir)
{
    const auto blindness = player_ptr->effects()->blindness();
    if (blindness->is_blind() || no_lite(player_ptr)) {
        msg_print(_("目が見えない！", "You cannot see!"));
        return 0;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    if ((disturb_trap_detect || alert_trap_detect) && player_ptr->dtrap && !(floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_IN_DETECT)) {
        player_ptr->dtrap = false;
        if (!(floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_UNSAFE)) {
            if (alert_trap_detect) {
                msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
            }

            if (disturb_trap_detect) {
                return 0;
            }
        }
    }

    int max = (prev_dir & 0x01) + 1;
    const grid_type *g_ptr;
    for (int i = -max; i <= max; i++) {
        DIRECTION dir = cycle[chome[prev_dir] + i];
        POSITION row = player_ptr->y + ddy[dir];
        POSITION col = player_ptr->x + ddx[dir];
        g_ptr = &floor_ptr->grid_array[row][col];
        if (g_ptr->m_idx) {
            auto *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
            if (m_ptr->ml) {
                return 0;
            }
        }
    }

    int cost = travel.cost[player_ptr->y][player_ptr->x];
    DIRECTION new_dir = 0;
    for (int i = 0; i < 8; ++i) {
        int dir_cost = travel.cost[player_ptr->y + ddy_ddd[i]][player_ptr->x + ddx_ddd[i]];
        if (dir_cost < cost) {
            new_dir = ddd[i];
            cost = dir_cost;
        }
    }

    if (!new_dir) {
        return 0;
    }

    g_ptr = &floor_ptr->grid_array[player_ptr->y + ddy[new_dir]][player_ptr->x + ddx[new_dir]];
    if (!easy_open && is_closed_door(player_ptr, g_ptr->feat)) {
        return 0;
    }

    if (!g_ptr->mimic && !trap_can_be_ignored(player_ptr, g_ptr->feat)) {
        return 0;
    }

    return new_dir;
}

/*!
 * @brief トラベル機能の実装 /
 * Travel command
 * @param player_ptr	プレイヤーへの参照ポインタ
 */
void travel_step(PlayerType *player_ptr)
{
    travel.dir = travel_test(player_ptr, travel.dir);
    if (!travel.dir) {
        if (travel.run == 255) {
            msg_print(_("道筋が見つかりません！", "No route is found!"));
            travel.y = travel.x = 0;
        }

        disturb(player_ptr, false, true);
        return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    exe_movement(player_ptr, travel.dir, always_pickup, false);
    if ((player_ptr->y == travel.y) && (player_ptr->x == travel.x)) {
        travel.run = 0;
        travel.y = travel.x = 0;
    } else if (travel.run > 0) {
        travel.run--;
    }

    term_xtra(TERM_XTRA_DELAY, delay_factor);
}

/*!
 * @brief トラベル処理の記憶配列を初期化する Hack: forget the "flow" information
 * @param player_ptr	プレイヤーへの参照ポインタ
 */
void forget_travel_flow(FloorType *floor_ptr)
{
    for (POSITION y = 0; y < floor_ptr->height; y++) {
        for (POSITION x = 0; x < floor_ptr->width; x++) {
            travel.cost[y][x] = MAX_SHORT;
        }
    }

    travel.y = travel.x = 0;
}
