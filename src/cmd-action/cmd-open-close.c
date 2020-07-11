#include "cmd-action/cmd-open-close.h"
#include "action/open-close-execution.h"
#include "action/open-util.h"
#include "cmd-action/cmd-attack.h"
#include "core/player-redraw-types.h"
#include "floor/floor.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/input-key-requester.h"
#include "io/targeting.h"
#include "player/attack-defense-types.h"
#include "player/player-move.h"
#include "player/special-defense-types.h"
#include "specific-object/chest.h"
#include "status/action-setter.h"
#include "status/experience.h"
#include "system/object-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 箱を開ける実行処理 /
 * Attempt to open the given chest at the given location
 * @param y 箱の存在するマスのY座標
 * @param x 箱の存在するマスのX座標
 * @param o_idx 箱のオブジェクトID
 * @return 箱が開かなかった場合TRUE / Returns TRUE if repeated commands may continue
 * @details
 * Assume there is no monster blocking the destination
 */
static bool exe_open_chest(player_type *creature_ptr, POSITION y, POSITION x, OBJECT_IDX o_idx)
{
    bool flag = TRUE;
    bool more = FALSE;
    object_type *o_ptr = &creature_ptr->current_floor_ptr->o_list[o_idx];
    take_turn(creature_ptr, 100);
    if (o_ptr->pval > 0) {
        flag = FALSE;
        int i = creature_ptr->skill_dis;
        if (creature_ptr->blind || no_lite(creature_ptr))
            i = i / 10;

        if (creature_ptr->confused || creature_ptr->image)
            i = i / 10;

        int j = i - o_ptr->pval;
        if (j < 2)
            j = 2;

        if (randint0(100) < j) {
            msg_print(_("鍵をはずした。", "You have picked the lock."));
            gain_exp(creature_ptr, 1);
            flag = TRUE;
        } else {
            more = TRUE;
            if (flush_failure)
                flush();

            msg_print(_("鍵をはずせなかった。", "You failed to pick the lock."));
        }
    }

    if (flag) {
        chest_trap(creature_ptr, y, x, o_idx);
        chest_death(creature_ptr, FALSE, y, x, o_idx);
    }

    return more;
}

/*!
 * @brief 「開ける」コマンドのメインルーチン /
 * Open a closed/locked/jammed door or a closed/locked chest.
 * @return なし
 * @details
 * Unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_open(player_type *creature_ptr)
{
    POSITION y, x;
    DIRECTION dir;
    OBJECT_IDX o_idx;
    bool more = FALSE;
    if (creature_ptr->wild_mode)
        return;

    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (easy_open) {
        int num_doors = count_dt(creature_ptr, &y, &x, is_closed_door, FALSE);
        int num_chests = count_chests(creature_ptr, &y, &x, FALSE);
        if (num_doors || num_chests) {
            bool too_many = (num_doors && num_chests) || (num_doors > 1) || (num_chests > 1);
            if (!too_many)
                command_dir = coords_to_dir(creature_ptr, y, x);
        }
    }

    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= PR_STATE;
        command_arg = 0;
    }

    if (get_rep_dir(creature_ptr, &dir, TRUE)) {
        FEAT_IDX feat;
        grid_type *g_ptr;
        y = creature_ptr->y + ddy[dir];
        x = creature_ptr->x + ddx[dir];
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
        feat = get_feat_mimic(g_ptr);
        o_idx = chest_check(creature_ptr->current_floor_ptr, y, x, FALSE);
        if (!have_flag(f_info[feat].flags, FF_OPEN) && !o_idx) {
            msg_print(_("そこには開けるものが見当たらない。", "You see nothing there to open."));
        } else if (g_ptr->m_idx && creature_ptr->riding != g_ptr->m_idx) {
            take_turn(creature_ptr, 100);
            msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
            do_cmd_attack(creature_ptr, y, x, 0);
        } else if (o_idx) {
            more = exe_open_chest(creature_ptr, y, x, o_idx);
        } else {
            more = exe_open(creature_ptr, y, x);
        }
    }

    if (!more)
        disturb(creature_ptr, FALSE, FALSE);
}

/*!
 * @brief 「閉じる」コマンドのメインルーチン /
 * Close an open door.
 * @return なし
 * @details
 * Unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_close(player_type *creature_ptr)
{
    POSITION y, x;
    DIRECTION dir;
    bool more = FALSE;
    if (creature_ptr->wild_mode)
        return;

    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (easy_open && (count_dt(creature_ptr, &y, &x, is_open, FALSE) == 1))
        command_dir = coords_to_dir(creature_ptr, y, x);

    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= (PR_STATE);
        command_arg = 0;
    }

    if (get_rep_dir(creature_ptr, &dir, FALSE)) {
        grid_type *g_ptr;
        FEAT_IDX feat;
        y = creature_ptr->y + ddy[dir];
        x = creature_ptr->x + ddx[dir];
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
        feat = get_feat_mimic(g_ptr);
        if (!have_flag(f_info[feat].flags, FF_CLOSE)) {
            msg_print(_("そこには閉じるものが見当たらない。", "You see nothing there to close."));
        } else if (g_ptr->m_idx) {
            take_turn(creature_ptr, 100);
            msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
            do_cmd_attack(creature_ptr, y, x, 0);
        } else {
            more = exe_close(creature_ptr, y, x);
        }
    }

    if (!more)
        disturb(creature_ptr, FALSE, FALSE);
}
