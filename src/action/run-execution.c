#include "action/run-execution.h"
#include "action/movement-execution.h"
#include "core/disturbance.h"
#include "floor/cave.h"
#include "floor/floor-util.h"
#include "game-option/disturbance-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object/object-mark-types.h"
#include "player/player-status-flags.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
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
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param dir 想定する移動方向ID
 * @param y 移動元のY座標
 * @param x 移動元のX座標
 * @return 移動先が既知の壁ならばTRUE
 */
static bool see_wall(player_type *creature_ptr, DIRECTION dir, POSITION y, POSITION x)
{
    y += ddy[dir];
    x += ddx[dir];
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (!in_bounds2(floor_ptr, y, x))
        return FALSE;

    grid_type *g_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (!(g_ptr->info & CAVE_MARK))
        return FALSE;

    s16b feat = get_feat_mimic(g_ptr);
    feature_type *f_ptr = &f_info[feat];
    if (!player_can_enter(creature_ptr, feat, 0))
        return !has_flag(f_ptr->flags, FF_DOOR);

    if (has_flag(f_ptr->flags, FF_AVOID_RUN) && !ignore_avoid_run)
        return TRUE;

    if (!has_flag(f_ptr->flags, FF_MOVE) && !has_flag(f_ptr->flags, FF_CAN_FLY))
        return !has_flag(f_ptr->flags, FF_DOOR);

    return FALSE;
}

/*!
 * @brief ダッシュ処理の導入 /
 * Initialize the running algorithm for a new direction.
 * @param creature_ptr	プレーヤーへの参照ポインタ
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
static void run_init(player_type *creature_ptr, DIRECTION dir)
{
    find_current = dir;
    find_prevdir = dir;
    find_openarea = TRUE;
    find_breakright = find_breakleft = FALSE;
    bool deepleft = FALSE;
    bool deepright = FALSE;
    bool shortright = FALSE;
    bool shortleft = FALSE;
    creature_ptr->run_py = creature_ptr->y;
    creature_ptr->run_px = creature_ptr->x;
    int row = creature_ptr->y + ddy[dir];
    int col = creature_ptr->x + ddx[dir];
    ignore_avoid_run = cave_has_flag_bold(creature_ptr->current_floor_ptr, row, col, FF_AVOID_RUN);
    int i = chome[dir];
    if (see_wall(creature_ptr, cycle[i + 1], creature_ptr->y, creature_ptr->x)) {
        find_breakleft = TRUE;
        shortleft = TRUE;
    } else if (see_wall(creature_ptr, cycle[i + 1], row, col)) {
        find_breakleft = TRUE;
        deepleft = TRUE;
    }

    if (see_wall(creature_ptr, cycle[i - 1], creature_ptr->y, creature_ptr->x)) {
        find_breakright = TRUE;
        shortright = TRUE;
    } else if (see_wall(creature_ptr, cycle[i - 1], row, col)) {
        find_breakright = TRUE;
        deepright = TRUE;
    }

    if (!find_breakleft || !find_breakright)
        return;

    find_openarea = FALSE;
    if (dir & 0x01) {
        if (deepleft && !deepright) {
            find_prevdir = cycle[i - 1];
        } else if (deepright && !deepleft) {
            find_prevdir = cycle[i + 1];
        }

        return;
    }

    if (!see_wall(creature_ptr, cycle[i], row, col))
        return;

    if (shortleft && !shortright) {
        find_prevdir = cycle[i - 2];
    } else if (shortright && !shortleft) {
        find_prevdir = cycle[i + 2];
    }
}

/*!
 * @brief ダッシュ移動処理中、移動先のマスか未知の地形かどうかを判定する /
 * Hack -- Check for an "unknown corner" (see below)
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param dir 想定する移動方向ID
 * @param y 移動元のY座標
 * @param x 移動元のX座標
 * @return 移動先が未知の地形ならばTRUE
 */
static bool see_nothing(player_type *creature_ptr, DIRECTION dir, POSITION y, POSITION x)
{
    y += ddy[dir];
    x += ddx[dir];

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (!in_bounds2(floor_ptr, y, x))
        return TRUE;

    if (floor_ptr->grid_array[y][x].info & (CAVE_MARK))
        return FALSE;

    if (player_can_see_bold(creature_ptr, y, x))
        return FALSE;

    return TRUE;
}

/*!
 * @brief ダッシュ移動が継続できるかどうかの判定 /
 * Update the current "run" path
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @return 立ち止まるべき条件が満たされたらTRUE
 * ダッシュ移動が継続できるならばTRUEを返す。
 * Return TRUE if the running should be stopped
 */
static bool run_test(player_type *creature_ptr)
{
    DIRECTION prev_dir = find_prevdir;
    int max = (prev_dir & 0x01) + 1;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if ((disturb_trap_detect || alert_trap_detect) && creature_ptr->dtrap && !(floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & CAVE_IN_DETECT)) {
        creature_ptr->dtrap = FALSE;
        if (!(floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & CAVE_UNSAFE)) {
            if (alert_trap_detect) {
                msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
            }

            if (disturb_trap_detect) {
                /* Break Run */
                return TRUE;
            }
        }
    }

    DIRECTION check_dir = 0;
    int option = 0, option2 = 0;
    for (int i = -max; i <= max; i++) {
        OBJECT_IDX this_o_idx, next_o_idx = 0;
        DIRECTION new_dir = cycle[chome[prev_dir] + i];
        int row = creature_ptr->y + ddy[new_dir];
        int col = creature_ptr->x + ddx[new_dir];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[row][col];
        FEAT_IDX feat = get_feat_mimic(g_ptr);
        feature_type *f_ptr;
        f_ptr = &f_info[feat];
        if (g_ptr->m_idx) {
            monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
            if (m_ptr->ml)
                return TRUE;
        }

        for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
            object_type *o_ptr;
            o_ptr = &floor_ptr->o_list[this_o_idx];
            next_o_idx = o_ptr->next_o_idx;
            if (o_ptr->marked & OM_FOUND)
                return TRUE;
        }

        bool inv = TRUE;
        if (g_ptr->info & (CAVE_MARK)) {
            bool notice = has_flag(f_ptr->flags, FF_NOTICE);
            if (notice && has_flag(f_ptr->flags, FF_MOVE)) {
                if (find_ignore_doors && has_flag(f_ptr->flags, FF_DOOR) && has_flag(f_ptr->flags, FF_CLOSE)) {
                    notice = FALSE;
                } else if (find_ignore_stairs && has_flag(f_ptr->flags, FF_STAIRS)) {
                    notice = FALSE;
                } else if (has_flag(f_ptr->flags, FF_LAVA) && (has_immune_fire(creature_ptr) || is_invuln(creature_ptr))) {
                    notice = FALSE;
                } else if (has_flag(f_ptr->flags, FF_WATER) && has_flag(f_ptr->flags, FF_DEEP)
                    && (creature_ptr->levitation || creature_ptr->can_swim || (calc_inventory_weight(creature_ptr) <= calc_weight_limit(creature_ptr)))) {
                    notice = FALSE;
                }
            }

            if (notice)
                return TRUE;

            inv = FALSE;
        }

        if (!inv && see_wall(creature_ptr, 0, row, col)) {
            if (find_openarea) {
                if (i < 0) {
                    find_breakright = TRUE;
                } else if (i > 0) {
                    find_breakleft = TRUE;
                }
            }

            continue;
        }

        if (find_openarea)
            continue;

        if (!option) {
            option = new_dir;
            continue;
        }

        if (option2)
            return TRUE;

        if (option != cycle[chome[prev_dir] + i - 1])
            return TRUE;

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
            if (!see_wall(creature_ptr, cycle[chome[prev_dir] + i], creature_ptr->y, creature_ptr->x)) {
                if (find_breakright)
                    return TRUE;
            } else {
                if (find_breakleft)
                    return TRUE;
            }
        }

        for (int i = max; i > 0; i--) {
            if (!see_wall(creature_ptr, cycle[chome[prev_dir] + i], creature_ptr->y, creature_ptr->x)) {
                if (find_breakleft)
                    return TRUE;
            } else {
                if (find_breakright)
                    return TRUE;
            }
        }

        return see_wall(creature_ptr, find_current, creature_ptr->y, creature_ptr->x);
    }

    if (!option)
        return TRUE;

    if (!option2) {
        find_current = option;
        find_prevdir = option;
        return see_wall(creature_ptr, find_current, creature_ptr->y, creature_ptr->x);
    } else if (!find_cut) {
        find_current = option;
        find_prevdir = option2;
        return see_wall(creature_ptr, find_current, creature_ptr->y, creature_ptr->x);
    }

    int row = creature_ptr->y + ddy[option];
    int col = creature_ptr->x + ddx[option];
    if (!see_wall(creature_ptr, option, row, col) || !see_wall(creature_ptr, check_dir, row, col)) {
        if (see_nothing(creature_ptr, option, row, col) && see_nothing(creature_ptr, option2, row, col)) {
            find_current = option;
            find_prevdir = option2;
            return see_wall(creature_ptr, find_current, creature_ptr->y, creature_ptr->x);
        }

        return TRUE;
    }

    if (find_cut) {
        find_current = option2;
        find_prevdir = option2;
        return see_wall(creature_ptr, find_current, creature_ptr->y, creature_ptr->x);
    }

    find_current = option;
    find_prevdir = option2;
    return see_wall(creature_ptr, find_current, creature_ptr->y, creature_ptr->x);
}

/*!
 * @brief 継続的なダッシュ処理 /
 * Take one step along the current "run" path
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param dir 移動を試みる方向ID
 * @return なし
 */
void run_step(player_type *creature_ptr, DIRECTION dir)
{
    if (dir) {
        ignore_avoid_run = TRUE;
        if (see_wall(creature_ptr, dir, creature_ptr->y, creature_ptr->x)) {
            sound(SOUND_HITWALL);
            msg_print(_("その方向には走れません。", "You cannot run in that direction."));
            disturb(creature_ptr, FALSE, FALSE);
            return;
        }

        run_init(creature_ptr, dir);
    } else {
        if (run_test(creature_ptr)) {
            disturb(creature_ptr, FALSE, FALSE);
            return;
        }
    }

    if (--creature_ptr->running <= 0)
        return;

    take_turn(creature_ptr, 100);
    exe_movement(creature_ptr, find_current, FALSE, FALSE);
    if (player_bold(creature_ptr, creature_ptr->run_py, creature_ptr->run_px)) {
        creature_ptr->run_py = 0;
        creature_ptr->run_px = 0;
        disturb(creature_ptr, FALSE, FALSE);
    }
}
