/*!
 * @file run-execution.cpp
 * @brief プレイヤーの走行処理実装
 */

#include "action/run-execution.h"
#include "action/movement-execution.h"
#include "core/disturbance.h"
#include "floor/cave.h"
#include "floor/floor-util.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object/object-mark-types.h"
#include "player-status/player-energy.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

bool ignore_avoid_run;

/* Allow quick "cycling" through the legal directions */
byte cycle[MAX_RUN_CYCLES] = { 1, 2, 3, 6, 9, 8, 7, 4, 1, 2, 3, 6, 9, 8, 7, 4, 1 };

/* Map each direction into the "middle" of the "cycle[]" array */
byte chome[MAX_RUN_CHOME] = { 0, 8, 9, 10, 7, 0, 11, 6, 5, 4 };

/* The direction we are running */
static DIRECTION find_current;

/* The direction we came from */
static DIRECTION find_prevdir;

static bool find_openarea;

/* We are looking for a break */
static bool find_breakright;
static bool find_breakleft;

/*!
 * @brief ダッシュ移動処理中、移動先のマスが既知の壁かどうかを判定する /
 * Hack -- Check for a "known wall" (see below)
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param dir 想定する移動方向ID
 * @param y 移動元のY座標
 * @param x 移動元のX座標
 * @return 移動先が既知の壁ならばTRUE
 */
static bool see_wall(PlayerType *player_ptr, DIRECTION dir, POSITION y, POSITION x)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    const Pos2D pos(y + ddy[dir], x + ddx[dir]);
    if (!in_bounds2(floor_ptr, pos.y, pos.x)) {
        return false;
    }

    const auto &grid = floor_ptr->get_grid(pos);
    if (!grid.is_mark()) {
        return false;
    }

    const auto feat = grid.get_feat_mimic();
    const auto &terrain = terrains_info[feat];
    if (!player_can_enter(player_ptr, feat, 0)) {
        return terrain.flags.has_not(TerrainCharacteristics::DOOR);
    }

    if (terrain.flags.has(TerrainCharacteristics::AVOID_RUN) && !ignore_avoid_run) {
        return true;
    }

    if (terrain.flags.has_none_of({ TerrainCharacteristics::MOVE, TerrainCharacteristics::CAN_FLY })) {
        return terrain.flags.has_not(TerrainCharacteristics::DOOR);
    }

    return false;
}

/*!
 * @brief ダッシュ処理の導入 /
 * Initialize the running algorithm for a new direction.
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param dir 導入の移動先
 * @details
 * Diagonal Corridor -- allow diaginal entry into corridors.\n
 *\n
 * Blunt Corridor -- If there is a wall two spaces ahead and\n
 * we seem to be in a corridor, then force a turn into the side\n
 * corridor, must be moving straight into a corridor here. ???\n
 *\n
 * Diagonal Corridor    Blunt Corridor (?)\n
 *       \# \#                  \#\n
 *       \#x\#                  \@x\#\n
 *       \@\@p.                  p\n
 */
static void run_init(PlayerType *player_ptr, DIRECTION dir)
{
    find_current = dir;
    find_prevdir = dir;
    find_openarea = true;
    find_breakright = find_breakleft = false;
    bool deepleft = false;
    bool deepright = false;
    bool shortright = false;
    bool shortleft = false;
    player_ptr->run_py = player_ptr->y;
    player_ptr->run_px = player_ptr->x;
    int row = player_ptr->y + ddy[dir];
    int col = player_ptr->x + ddx[dir];
    ignore_avoid_run = cave_has_flag_bold(player_ptr->current_floor_ptr, row, col, TerrainCharacteristics::AVOID_RUN);
    int i = chome[dir];
    if (see_wall(player_ptr, cycle[i + 1], player_ptr->y, player_ptr->x)) {
        find_breakleft = true;
        shortleft = true;
    } else if (see_wall(player_ptr, cycle[i + 1], row, col)) {
        find_breakleft = true;
        deepleft = true;
    }

    if (see_wall(player_ptr, cycle[i - 1], player_ptr->y, player_ptr->x)) {
        find_breakright = true;
        shortright = true;
    } else if (see_wall(player_ptr, cycle[i - 1], row, col)) {
        find_breakright = true;
        deepright = true;
    }

    if (!find_breakleft || !find_breakright) {
        return;
    }

    find_openarea = false;
    if (dir & 0x01) {
        if (deepleft && !deepright) {
            find_prevdir = cycle[i - 1];
        } else if (deepright && !deepleft) {
            find_prevdir = cycle[i + 1];
        }

        return;
    }

    if (!see_wall(player_ptr, cycle[i], row, col)) {
        return;
    }

    if (shortleft && !shortright) {
        find_prevdir = cycle[i - 2];
    } else if (shortright && !shortleft) {
        find_prevdir = cycle[i + 2];
    }
}

/*!
 * @brief ダッシュ移動処理中、移動先のマスか未知の地形かどうかを判定する /
 * Hack -- Check for an "unknown corner" (see below)
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param dir 想定する移動方向ID
 * @param y 移動元のY座標
 * @param x 移動元のX座標
 * @return 移動先が未知の地形ならばTRUE
 */
static bool see_nothing(PlayerType *player_ptr, DIRECTION dir, POSITION y, POSITION x)
{
    y += ddy[dir];
    x += ddx[dir];

    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!in_bounds2(floor_ptr, y, x)) {
        return true;
    }

    if (floor_ptr->grid_array[y][x].is_mark()) {
        return false;
    }

    if (player_can_see_bold(player_ptr, y, x)) {
        return false;
    }

    return true;
}

/*!
 * @brief ダッシュ移動が継続できるかどうかの判定 /
 * Update the current "run" path
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @return 立ち止まるべき条件が満たされたらTRUE
 * ダッシュ移動が継続できるならばTRUEを返す。
 * Return TRUE if the running should be stopped
 */
static bool run_test(PlayerType *player_ptr)
{
    const auto prev_dir = find_prevdir;
    const auto max = (prev_dir & 0x01) + 1;
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    const auto &p_grid = floor.get_grid(p_pos);
    if ((disturb_trap_detect || alert_trap_detect) && player_ptr->dtrap && !(p_grid.info & CAVE_IN_DETECT)) {
        player_ptr->dtrap = false;
        if (!(p_grid.info & CAVE_UNSAFE)) {
            if (alert_trap_detect) {
                msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
            }

            if (disturb_trap_detect) {
                /* Break Run */
                return true;
            }
        }
    }

    auto check_dir = 0;
    auto option = 0;
    auto option2 = 0;
    for (auto i = -max; i <= max; i++) {
        int new_dir = cycle[chome[prev_dir] + i];
        const Pos2D pos(player_ptr->y + ddy[new_dir], player_ptr->x + ddx[new_dir]);
        const auto &grid = floor.get_grid(pos);
        const auto &terrain = terrains_info[grid.get_feat_mimic()];
        if (grid.m_idx) {
            const auto &monster = floor.m_list[grid.m_idx];
            if (monster.ml) {
                return true;
            }
        }

        for (const auto this_o_idx : grid.o_idx_list) {
            const auto &item = floor.o_list[this_o_idx];
            if (item.marked.has(OmType::FOUND)) {
                return true;
            }
        }

        auto inv = true;
        if (grid.is_mark()) {
            auto notice = terrain.flags.has(TerrainCharacteristics::NOTICE);
            if (notice && terrain.flags.has(TerrainCharacteristics::MOVE)) {
                if (find_ignore_doors && terrain.flags.has_all_of({ TerrainCharacteristics::DOOR, TerrainCharacteristics::CLOSE })) {
                    notice = false;
                } else if (find_ignore_stairs && terrain.flags.has(TerrainCharacteristics::STAIRS)) {
                    notice = false;
                } else if (terrain.flags.has(TerrainCharacteristics::LAVA) && (has_immune_fire(player_ptr) || is_invuln(player_ptr))) {
                    notice = false;
                } else if (terrain.flags.has_all_of({ TerrainCharacteristics::WATER, TerrainCharacteristics::DEEP }) && (player_ptr->levitation || player_ptr->can_swim || (calc_inventory_weight(player_ptr) <= calc_weight_limit(player_ptr)))) {
                    notice = false;
                }
            }

            if (notice) {
                return true;
            }

            inv = false;
        }

        if (!inv && see_wall(player_ptr, 0, pos.y, pos.x)) {
            if (find_openarea) {
                if (i < 0) {
                    find_breakright = true;
                } else if (i > 0) {
                    find_breakleft = true;
                }
            }

            continue;
        }

        if (find_openarea) {
            continue;
        }

        if (!option) {
            option = new_dir;
            continue;
        }

        if (option2) {
            return true;
        }

        if (option != cycle[chome[prev_dir] + i - 1]) {
            return true;
        }

        if (new_dir & 0x01) {
            check_dir = cycle[chome[prev_dir] + i - 2];
            option2 = new_dir;
            continue;
        }

        check_dir = cycle[chome[prev_dir] + i + 1];
        option2 = option;
        option = new_dir;
    }

    if (find_openarea) {
        for (int i = -max; i < 0; i++) {
            if (!see_wall(player_ptr, cycle[chome[prev_dir] + i], player_ptr->y, player_ptr->x)) {
                if (find_breakright) {
                    return true;
                }
            } else {
                if (find_breakleft) {
                    return true;
                }
            }
        }

        for (int i = max; i > 0; i--) {
            if (!see_wall(player_ptr, cycle[chome[prev_dir] + i], player_ptr->y, player_ptr->x)) {
                if (find_breakleft) {
                    return true;
                }
            } else {
                if (find_breakright) {
                    return true;
                }
            }
        }

        return see_wall(player_ptr, find_current, player_ptr->y, player_ptr->x);
    }

    if (!option) {
        return true;
    }

    if (!option2) {
        find_current = option;
        find_prevdir = option;
        return see_wall(player_ptr, find_current, player_ptr->y, player_ptr->x);
    } else if (!find_cut) {
        find_current = option;
        find_prevdir = option2;
        return see_wall(player_ptr, find_current, player_ptr->y, player_ptr->x);
    }

    int row = player_ptr->y + ddy[option];
    int col = player_ptr->x + ddx[option];
    if (!see_wall(player_ptr, option, row, col) || !see_wall(player_ptr, check_dir, row, col)) {
        if (see_nothing(player_ptr, option, row, col) && see_nothing(player_ptr, option2, row, col)) {
            find_current = option;
            find_prevdir = option2;
            return see_wall(player_ptr, find_current, player_ptr->y, player_ptr->x);
        }

        return true;
    }

    if (find_cut) {
        find_current = option2;
        find_prevdir = option2;
        return see_wall(player_ptr, find_current, player_ptr->y, player_ptr->x);
    }

    find_current = option;
    find_prevdir = option2;
    return see_wall(player_ptr, find_current, player_ptr->y, player_ptr->x);
}

/*!
 * @brief 継続的なダッシュ処理 /
 * Take one step along the current "run" path
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param dir 移動を試みる方向ID
 */
void run_step(PlayerType *player_ptr, DIRECTION dir)
{
    if (dir) {
        ignore_avoid_run = true;
        if (see_wall(player_ptr, dir, player_ptr->y, player_ptr->x)) {
            sound(SOUND_HITWALL);
            msg_print(_("その方向には走れません。", "You cannot run in that direction."));
            disturb(player_ptr, false, false);
            return;
        }

        run_init(player_ptr, dir);
    } else {
        if (run_test(player_ptr)) {
            disturb(player_ptr, false, false);
            return;
        }
    }

    if (--player_ptr->running <= 0) {
        return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    exe_movement(player_ptr, find_current, false, false);
    if (player_ptr->is_located_at_running_destination()) {
        player_ptr->run_py = 0;
        player_ptr->run_px = 0;
        disturb(player_ptr, false, false);
    }
}
