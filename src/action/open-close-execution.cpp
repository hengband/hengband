﻿/*!
 * @brief 扉や箱を開ける処理
 * @date 2020/07/11
 * @author Hourier
 */

#include "action/open-close-execution.h"
#include "action/movement-execution.h"
#include "combat/attack-power-table.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "perception/object-perception.h"
#include "player/player-status-table.h"
#include "specific-object/chest.h"
#include "status/bad-status-setter.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 「開ける」動作コマンドのサブルーチン /
 * Perform the basic "open" command on doors
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return 連続でコマンドを実行する時のみTRUE、1回きりの時はFALSE
 */
bool exe_open(player_type *creature_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    take_turn(creature_ptr, 100);
    if (!has_flag(f_ptr->flags, FF_OPEN)) {
        msg_format(_("%sはがっちりと閉じられているようだ。", "The %s appears to be stuck."), f_name + f_info[get_feat_mimic(g_ptr)].name);
        return FALSE;
    }

    if (!f_ptr->power) {
        cave_alter_feat(creature_ptr, y, x, FF_OPEN);
        sound(SOUND_OPENDOOR);
        return FALSE;
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
        return TRUE;
    }

    msg_print(_("鍵をはずした。", "You have picked the lock."));
    cave_alter_feat(creature_ptr, y, x, FF_OPEN);
    sound(SOUND_OPENDOOR);
    gain_exp(creature_ptr, 1);
    return FALSE;
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
    if (!has_flag(f_info[old_feat].flags, FF_CLOSE))
        return more;

    s16b closed_feat = feat_state(creature_ptr, old_feat, FF_CLOSE);
    if ((g_ptr->o_idx || (g_ptr->info & CAVE_OBJECT)) && (closed_feat != old_feat) && !has_flag(f_info[closed_feat].flags, FF_DROP)) {
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

    if (!has_flag(f_ptr->flags, FF_OPEN)) {
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

/*!
 * @brief 箱のトラップを解除する実行処理 /
 * Perform the basic "disarm" command
 * @param y 解除を行うマスのY座標
 * @param x 解除を行うマスのX座標
 * @param o_idx 箱のオブジェクトID
 * @return ターンを消費する処理が行われた場合TRUEを返す
 * @details
 * <pre>
 * Assume destination is a visible trap
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 * </pre>
 */
bool exe_disarm_chest(player_type *creature_ptr, POSITION y, POSITION x, OBJECT_IDX o_idx)
{
    bool more = FALSE;
    object_type *o_ptr = &creature_ptr->current_floor_ptr->o_list[o_idx];
    take_turn(creature_ptr, 100);
    int i = creature_ptr->skill_dis;
    if (creature_ptr->blind || no_lite(creature_ptr))
        i = i / 10;

    if (creature_ptr->confused || creature_ptr->image)
        i = i / 10;

    int j = i - o_ptr->pval;
    if (j < 2)
        j = 2;

    if (!object_is_known(o_ptr)) {
        msg_print(_("トラップが見あたらない。", "I don't see any traps."));
    } else if (o_ptr->pval <= 0) {
        msg_print(_("箱にはトラップが仕掛けられていない。", "The chest is not trapped."));
    } else if (!chest_traps[o_ptr->pval]) {
        msg_print(_("箱にはトラップが仕掛けられていない。", "The chest is not trapped."));
    } else if (randint0(100) < j) {
        msg_print(_("箱に仕掛けられていたトラップを解除した。", "You have disarmed the chest."));
        gain_exp(creature_ptr, o_ptr->pval);
        o_ptr->pval = (0 - o_ptr->pval);
    } else if ((i > 5) && (randint1(i) > 5)) {
        more = TRUE;
        if (flush_failure)
            flush();

        msg_print(_("箱のトラップ解除に失敗した。", "You failed to disarm the chest."));
    } else {
        msg_print(_("トラップを作動させてしまった！", "You set off a trap!"));
        sound(SOUND_FAIL);
        chest_trap(creature_ptr, y, x, o_idx);
    }

    return more;
}

/*!
 * @brief 箱のトラップを解除するコマンドのサブルーチン /
 * Perform the basic "disarm" command
 * @param y 解除を行うマスのY座標
 * @param x 解除を行うマスのX座標
 * @param dir プレイヤーからみた方向ID
 * @return ターンを消費する処理が行われた場合TRUEを返す
 * @details
 * <pre>
 * Assume destination is a visible trap
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 * </pre>
 */

bool exe_disarm(player_type *creature_ptr, POSITION y, POSITION x, DIRECTION dir)
{
    grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    concptr name = (f_name + f_ptr->name);
    int power = f_ptr->power;
    bool more = FALSE;
    int i = creature_ptr->skill_dis;
    take_turn(creature_ptr, 100);
    if (creature_ptr->blind || no_lite(creature_ptr))
        i = i / 10;

    if (creature_ptr->confused || creature_ptr->image)
        i = i / 10;

    int j = i - power;
    if (j < 2)
        j = 2;

    if (randint0(100) < j) {
        msg_format(_("%sを解除した。", "You have disarmed the %s."), name);
        gain_exp(creature_ptr, power);
        cave_alter_feat(creature_ptr, y, x, FF_DISARM);
        exe_movement(creature_ptr, dir, easy_disarm, FALSE);
    } else if ((i > 5) && (randint1(i) > 5)) {
        if (flush_failure)
            flush();

        msg_format(_("%sの解除に失敗した。", "You failed to disarm the %s."), name);
        more = TRUE;
    } else {
        msg_format(_("%sを作動させてしまった！", "You set off the %s!"), name);
        exe_movement(creature_ptr, dir, easy_disarm, FALSE);
    }

    return more;
}

/*!
 * @brief 「打ち破る」動作コマンドのサブルーチン /
 * Perform the basic "bash" command
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @param dir プレイヤーから見たターゲットの方角ID
 * @return 実際に処理が行われた場合TRUEを返す。
 * @details
 * <pre>
 * Assume destination is a closed/locked/jammed door
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 * </pre>
 */
bool exe_bash(player_type *creature_ptr, POSITION y, POSITION x, DIRECTION dir)
{
    grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    int bash = adj_str_blow[creature_ptr->stat_ind[A_STR]];
    int temp = f_ptr->power;
    bool more = FALSE;
    concptr name = f_name + f_info[get_feat_mimic(g_ptr)].name;
    take_turn(creature_ptr, 100);
    msg_format(_("%sに体当たりをした！", "You smash into the %s!"), name);
    temp = (bash - (temp * 10));
    if (creature_ptr->pclass == CLASS_BERSERKER)
        temp *= 2;

    if (temp < 1)
        temp = 1;

    if (randint0(100) < temp) {
        msg_format(_("%sを壊した！", "The %s crashes open!"), name);
        sound(has_flag(f_ptr->flags, FF_GLASS) ? SOUND_GLASS : SOUND_OPENDOOR);
        if ((randint0(100) < 50) || (feat_state(creature_ptr, g_ptr->feat, FF_OPEN) == g_ptr->feat) || has_flag(f_ptr->flags, FF_GLASS)) {
            cave_alter_feat(creature_ptr, y, x, FF_BASH);
        } else {
            cave_alter_feat(creature_ptr, y, x, FF_OPEN);
        }

        exe_movement(creature_ptr, dir, FALSE, FALSE);
    } else if (randint0(100) < adj_dex_safe[creature_ptr->stat_ind[A_DEX]] + creature_ptr->lev) {
        msg_format(_("この%sは頑丈だ。", "The %s holds firm."), name);
        more = TRUE;
    } else {
        msg_print(_("体のバランスをくずしてしまった。", "You are off-balance."));
        (void)set_paralyzed(creature_ptr, creature_ptr->paralyzed + 2 + randint0(2));
    }

    return more;
}