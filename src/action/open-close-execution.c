#include "action/open-close-execution.h"
#include "floor/floor.h"
#include "game-option/disturbance-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "status/experience.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 「開ける」動作コマンドのサブルーチン /
 * Perform the basic "open" command on doors
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return 実際に処理が行われた場合TRUEを返す。
 * @details
 * Assume destination is a closed/locked/jammed door
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 */
bool exe_open(player_type *creature_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    bool more = FALSE;
    take_turn(creature_ptr, 100);
    if (!have_flag(f_ptr->flags, FF_OPEN)) {
        msg_format(_("%sはがっちりと閉じられているようだ。", "The %s appears to be stuck."), f_name + f_info[get_feat_mimic(g_ptr)].name);
        return more;
    }

    if (!f_ptr->power) {
        cave_alter_feat(creature_ptr, y, x, FF_OPEN);
        sound(SOUND_OPENDOOR);
        return more;
    }

    int i = creature_ptr->skill_dis;
    if (creature_ptr->blind || no_lite(creature_ptr))
        i = i / 10;

    if (creature_ptr->confused || creature_ptr->image)
        i = i / 10;

    int j = f_ptr->power;
    j = i - (j * 4);
    if (j < 2)
        j = 2;

    if (randint0(100) >= j) {
        if (flush_failure)
            flush();

        msg_print(_("鍵をはずせなかった。", "You failed to pick the lock."));
        more = TRUE;
    }

    msg_print(_("鍵をはずした。", "You have picked the lock."));
    cave_alter_feat(creature_ptr, y, x, FF_OPEN);
    sound(SOUND_OPENDOOR);
    gain_exp(creature_ptr, 1);
    return more;
}

/*
 * todo 常にFALSEを返している
 * @brief 「閉じる」動作コマンドのサブルーチン /
 * Perform the basic "close" command
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return 実際に処理が行われた場合TRUEを返す。
 * @details
 * Assume destination is an open/broken door
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 */
bool exe_close(player_type *creature_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    FEAT_IDX old_feat = g_ptr->feat;
    bool more = FALSE;
    take_turn(creature_ptr, 100);
    if (!have_flag(f_info[old_feat].flags, FF_CLOSE))
        return more;

    s16b closed_feat = feat_state(creature_ptr, old_feat, FF_CLOSE);
    if ((g_ptr->o_idx || (g_ptr->info & CAVE_OBJECT)) && (closed_feat != old_feat) && !have_flag(f_info[closed_feat].flags, FF_DROP)) {
        msg_print(_("何かがつっかえて閉まらない。", "Something prevents it from closing."));
    } else {
        cave_alter_feat(creature_ptr, y, x, FF_CLOSE);
        if (old_feat == g_ptr->feat) {
            msg_print(_("ドアは壊れてしまっている。", "The door appears to be broken."));
        } else {
            sound(SOUND_SHUTDOOR);
        }
    }

    return more;
}

/*!
 * @brief 移動処理による簡易な「開く」処理 /
 * easy_open_door --
 * @return 開く処理が実際に試みられた場合TRUEを返す
 * @details
 * <pre>
 *	If there is a jammed/closed/locked door at the given location,
 *	then attempt to unlock/open it. Return TRUE if an attempt was
 *	made (successful or not), otherwise return FALSE.
 *
 *	The code here should be nearly identical to that in
 *	do_cmd_open_test() and exe_open().
 * </pre>
 */
bool easy_open_door(player_type *creature_ptr, POSITION y, POSITION x)
{
    int i, j;
    grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    if (!is_closed_door(creature_ptr, g_ptr->feat))
        return FALSE;

    if (!have_flag(f_ptr->flags, FF_OPEN)) {
        msg_format(_("%sはがっちりと閉じられているようだ。", "The %s appears to be stuck."), f_name + f_info[get_feat_mimic(g_ptr)].name);
    } else if (f_ptr->power) {
        i = creature_ptr->skill_dis;
        if (creature_ptr->blind || no_lite(creature_ptr))
            i = i / 10;

        if (creature_ptr->confused || creature_ptr->image)
            i = i / 10;

        j = f_ptr->power;
        j = i - (j * 4);
        if (j < 2)
            j = 2;

        if (randint0(100) < j) {
            msg_print(_("鍵をはずした。", "You have picked the lock."));
            cave_alter_feat(creature_ptr, y, x, FF_OPEN);
            sound(SOUND_OPENDOOR);
            gain_exp(creature_ptr, 1);
        } else {
            if (flush_failure)
                flush();

            msg_print(_("鍵をはずせなかった。", "You failed to pick the lock."));
        }
    } else {
        cave_alter_feat(creature_ptr, y, x, FF_OPEN);
        sound(SOUND_OPENDOOR);
    }

    return TRUE;
}
