/*!
 *  @brief プレイヤーのコマンド処理2 / Movement commands (part 2)
 *  @date 2014/01/02
 *  @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "cmd/cmd-basic.h"
#include "action/action-limited.h"
#include "action/open-close-execution.h"
#include "action/tunnel-execution.h"
#include "action/open-util.h"
#include "art-definition/art-weapon-types.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-save.h"
#include "combat/attack-power-table.h"
#include "combat/shoot.h"
#include "combat/slaying.h"
#include "core/asking-player.h"
#include "core/output-updater.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/spells-effect-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "game-option/disturbance-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "info-reader/fixed-map-parser.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/player-inventory.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/screen-util.h"
#include "io/targeting.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "mind/racial-android.h"
#include "mind/snipe-types.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-expendable.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-broken.h"
#include "object/object-flags.h"
#include "object/object-generator.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-stack.h"
#include "perception/object-perception.h"
#include "player/attack-defense-types.h"
#include "player/avatar.h"
#include "player/player-move.h"
#include "player/player-personalities-types.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "specific-object/chest.h"
#include "specific-object/torch.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/experience.h"
#include "sv-definition/sv-bow-types.h"
#include "system/system-variables.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"

/*!
 * @brief 探索コマンドのメインルーチン / Simple command to "search" for one turn
 * @return なし
 */
void do_cmd_search(player_type *creature_ptr)
{
    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= (PR_STATE);
        command_arg = 0;
    }

    take_turn(creature_ptr, 100);
    search(creature_ptr);
}

/*!
 * @brief 「掘る」動作コマンドのメインルーチン /
 * Tunnels through "walls" (including rubble and closed doors)
 * @return なし
 * @details
 * <pre>
 * Note that you must tunnel in order to hit invisible monsters
 * in walls, though moving into walls still takes a turn anyway.
 *
 * Digging is very difficult without a "digger" weapon, but can be
 * accomplished by strong players using heavy weapons.
 * </pre>
 */
void do_cmd_tunnel(player_type *creature_ptr)
{
    POSITION y, x;
    DIRECTION dir;
    grid_type *g_ptr;
    FEAT_IDX feat;
    bool more = FALSE;
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= (PR_STATE);
        command_arg = 0;
    }

    if (get_rep_dir(creature_ptr, &dir, FALSE)) {
        y = creature_ptr->y + ddy[dir];
        x = creature_ptr->x + ddx[dir];
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
        feat = get_feat_mimic(g_ptr);
        if (have_flag(f_info[feat].flags, FF_DOOR)) {
            msg_print(_("ドアは掘れない。", "You cannot tunnel through doors."));
        } else if (!have_flag(f_info[feat].flags, FF_TUNNEL)) {
            msg_print(_("そこは掘れない。", "You can't tunnel through that."));
        } else if (g_ptr->m_idx) {
            take_turn(creature_ptr, 100);
            msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
            do_cmd_attack(creature_ptr, y, x, 0);
        } else {
            more = exe_tunnel(creature_ptr, y, x);
        }
    }

    if (!more)
        disturb(creature_ptr, FALSE, FALSE);
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
static bool exe_disarm_chest(player_type *creature_ptr, POSITION y, POSITION x, OBJECT_IDX o_idx)
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
        move_player(creature_ptr, dir, easy_disarm, FALSE);
    } else if ((i > 5) && (randint1(i) > 5)) {
        if (flush_failure)
            flush();

        msg_format(_("%sの解除に失敗した。", "You failed to disarm the %s."), name);
        more = TRUE;
    } else {
        msg_format(_("%sを作動させてしまった！", "You set off the %s!"), name);
        move_player(creature_ptr, dir, easy_disarm, FALSE);
    }

    return more;
}

/*!
 * @brief 箱、床のトラップ解除処理双方の統合メインルーチン /
 * Disarms a trap, or chest
 * @return なし
 */
void do_cmd_disarm(player_type *creature_ptr)
{
    POSITION y, x;
    DIRECTION dir;
    OBJECT_IDX o_idx;
    bool more = FALSE;
    if (creature_ptr->wild_mode)
        return;

    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (easy_disarm) {
        int num_traps = count_dt(creature_ptr, &y, &x, is_trap, TRUE);
        int num_chests = count_chests(creature_ptr, &y, &x, TRUE);
        if (num_traps || num_chests) {
            bool too_many = (num_traps && num_chests) || (num_traps > 1) || (num_chests > 1);
            if (!too_many)
                command_dir = coords_to_dir(creature_ptr, y, x);
        }
    }

    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= (PR_STATE);
        command_arg = 0;
    }

    if (get_rep_dir(creature_ptr, &dir, TRUE)) {
        grid_type *g_ptr;
        FEAT_IDX feat;
        y = creature_ptr->y + ddy[dir];
        x = creature_ptr->x + ddx[dir];
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
        feat = get_feat_mimic(g_ptr);
        o_idx = chest_check(creature_ptr->current_floor_ptr, y, x, TRUE);
        if (!is_trap(creature_ptr, feat) && !o_idx) {
            msg_print(_("そこには解除するものが見当たらない。", "You see nothing there to disarm."));
        } else if (g_ptr->m_idx && creature_ptr->riding != g_ptr->m_idx) {
            msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
            do_cmd_attack(creature_ptr, y, x, 0);
        } else if (o_idx) {
            more = exe_disarm_chest(creature_ptr, y, x, o_idx);
        } else {
            more = exe_disarm(creature_ptr, y, x, dir);
        }
    }

    if (!more)
        disturb(creature_ptr, FALSE, FALSE);
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
static bool do_cmd_bash_aux(player_type *creature_ptr, POSITION y, POSITION x, DIRECTION dir)
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
        sound(have_flag(f_ptr->flags, FF_GLASS) ? SOUND_GLASS : SOUND_OPENDOOR);
        if ((randint0(100) < 50) || (feat_state(creature_ptr, g_ptr->feat, FF_OPEN) == g_ptr->feat) || have_flag(f_ptr->flags, FF_GLASS)) {
            cave_alter_feat(creature_ptr, y, x, FF_BASH);
        } else {
            cave_alter_feat(creature_ptr, y, x, FF_OPEN);
        }

        move_player(creature_ptr, dir, FALSE, FALSE);
    } else if (randint0(100) < adj_dex_safe[creature_ptr->stat_ind[A_DEX]] + creature_ptr->lev) {
        msg_format(_("この%sは頑丈だ。", "The %s holds firm."), name);
        more = TRUE;
    } else {
        msg_print(_("体のバランスをくずしてしまった。", "You are off-balance."));
        (void)set_paralyzed(creature_ptr, creature_ptr->paralyzed + 2 + randint0(2));
    }

    return more;
}

/*!
 * @brief 「打ち破る」動作コマンドのメインルーチン /
 * Bash open a door, success based on character strength
 * @return なし
 * @details
 * <pre>
 * For a closed door, pval is positive if locked; negative if stuck.
 *
 * For an open door, pval is positive for a broken door.
 *
 * A closed door can be opened - harder if locked. Any door might be
 * bashed open (and thereby broken). Bashing a door is (potentially)
 * faster! You move into the door way. To open a stuck door, it must
 * be bashed. A closed door can be jammed (see do_cmd_spike()).
 *
 * Creatures can also open or bash doors, see elsewhere.
 * </pre>
 */
void do_cmd_bash(player_type *creature_ptr)
{
    POSITION y, x;
    DIRECTION dir;
    grid_type *g_ptr;
    bool more = FALSE;
    if (creature_ptr->wild_mode)
        return;

    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= (PR_STATE);
        command_arg = 0;
    }

    if (get_rep_dir(creature_ptr, &dir, FALSE)) {
        FEAT_IDX feat;
        y = creature_ptr->y + ddy[dir];
        x = creature_ptr->x + ddx[dir];
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
        feat = get_feat_mimic(g_ptr);
        if (!have_flag(f_info[feat].flags, FF_BASH)) {
            msg_print(_("そこには体当たりするものが見当たらない。", "You see nothing there to bash."));
        } else if (g_ptr->m_idx) {
            take_turn(creature_ptr, 100);
            msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
            do_cmd_attack(creature_ptr, y, x, 0);
        } else {
            more = do_cmd_bash_aux(creature_ptr, y, x, dir);
        }
    }

    if (!more)
        disturb(creature_ptr, FALSE, FALSE);
}

/*!
 * @brief 特定のマスに影響を及ぼすための汎用的コマンド
 * @return なし
 * @details
 * <pre>
 * Manipulate an adjacent grid in some way
 *
 * Attack monsters, tunnel through walls, disarm traps, open doors.
 *
 * Consider confusion
 *
 * This command must always take a turn, to prevent free detection
 * of invisible monsters.
 * </pre>
 */
void do_cmd_alter(player_type *creature_ptr)
{
    POSITION y, x;
    DIRECTION dir;
    grid_type *g_ptr;
    bool more = FALSE;
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= (PR_STATE);
        command_arg = 0;
    }

    if (get_rep_dir(creature_ptr, &dir, TRUE)) {
        FEAT_IDX feat;
        feature_type *f_ptr;
        y = creature_ptr->y + ddy[dir];
        x = creature_ptr->x + ddx[dir];
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
        feat = get_feat_mimic(g_ptr);
        f_ptr = &f_info[feat];
        take_turn(creature_ptr, 100);
        if (g_ptr->m_idx) {
            do_cmd_attack(creature_ptr, y, x, 0);
        } else if (have_flag(f_ptr->flags, FF_OPEN)) {
            more = exe_open(creature_ptr, y, x);
        } else if (have_flag(f_ptr->flags, FF_BASH)) {
            more = do_cmd_bash_aux(creature_ptr, y, x, dir);
        } else if (have_flag(f_ptr->flags, FF_TUNNEL)) {
            more = exe_tunnel(creature_ptr, y, x);
        } else if (have_flag(f_ptr->flags, FF_CLOSE)) {
            more = exe_close(creature_ptr, y, x);
        } else if (have_flag(f_ptr->flags, FF_DISARM)) {
            more = exe_disarm(creature_ptr, y, x, dir);
        } else {
            msg_print(_("何もない空中を攻撃した。", "You attack the empty air."));
        }
    }

    if (!more)
        disturb(creature_ptr, FALSE, FALSE);
}

/*!
 * @brief 「くさびを打つ」ために必要なオブジェクトを所持しているかどうかの判定を返す /
 * Find the index of some "spikes", if possible.
 * @param ip くさびとして打てるオブジェクトのID
 * @return オブジェクトがある場合TRUEを返す
 * @details
 * <pre>
 * Let user choose a pile of spikes, perhaps?
 * </pre>
 */
static bool get_spike(player_type *creature_ptr, INVENTORY_IDX *ip)
{
    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        object_type *o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        if (o_ptr->tval == TV_SPIKE) {
            *ip = i;
            return TRUE;
        }
    }

    return FALSE;
}

/*!
 * @brief 「くさびを打つ」動作コマンドのメインルーチン /
 * Jam a closed door with a spike
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * This command may NOT be repeated
 * </pre>
 */
void do_cmd_spike(player_type *creature_ptr)
{
    DIRECTION dir;
    if (creature_ptr->wild_mode)
        return;

    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (!get_rep_dir(creature_ptr, &dir, FALSE))
        return;

    POSITION y = creature_ptr->y + ddy[dir];
    POSITION x = creature_ptr->x + ddx[dir];
    grid_type *g_ptr;
    g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    FEAT_IDX feat = get_feat_mimic(g_ptr);
    INVENTORY_IDX item;
    if (!have_flag(f_info[feat].flags, FF_SPIKE)) {
        msg_print(_("そこにはくさびを打てるものが見当たらない。", "You see nothing there to spike."));
    } else if (!get_spike(creature_ptr, &item)) {
        msg_print(_("くさびを持っていない！", "You have no spikes!"));
    } else if (g_ptr->m_idx) {
        take_turn(creature_ptr, 100);
        msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
        do_cmd_attack(creature_ptr, y, x, 0);
    } else {
        take_turn(creature_ptr, 100);
        msg_format(_("%sにくさびを打ち込んだ。", "You jam the %s with a spike."), f_name + f_info[feat].name);
        cave_alter_feat(creature_ptr, y, x, FF_SPIKE);
        vary_item(creature_ptr, item, -1);
    }
}

/*!
 * @brief 「歩く」動作コマンドのメインルーチン /
 * Support code for the "Walk" and "Jump" commands
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param pickup アイテムの自動拾いを行うならTRUE
 * @return なし
 */
void do_cmd_walk(player_type *creature_ptr, bool pickup)
{
    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= (PR_STATE);
        command_arg = 0;
    }

    bool more = FALSE;
    DIRECTION dir;
    if (get_rep_dir(creature_ptr, &dir, FALSE)) {
        take_turn(creature_ptr, 100);
        if ((dir != 5) && (creature_ptr->special_defense & KATA_MUSOU))
            set_action(creature_ptr, ACTION_NONE);

        if (creature_ptr->wild_mode)
            creature_ptr->energy_use *= ((MAX_HGT + MAX_WID) / 2);

        if (creature_ptr->action == ACTION_HAYAGAKE)
            creature_ptr->energy_use = creature_ptr->energy_use * (45 - (creature_ptr->lev / 2)) / 100;

        move_player(creature_ptr, dir, pickup, FALSE);
        more = TRUE;
    }

    if (creature_ptr->wild_mode && !cave_have_flag_bold(creature_ptr->current_floor_ptr, creature_ptr->y, creature_ptr->x, FF_TOWN)) {
        int tmp = 120 + creature_ptr->lev * 10 - wilderness[creature_ptr->y][creature_ptr->x].level + 5;
        if (tmp < 1)
            tmp = 1;

        if (((wilderness[creature_ptr->y][creature_ptr->x].level + 5) > (creature_ptr->lev / 2)) && randint0(tmp) < (21 - creature_ptr->skill_stl)) {
            msg_print(_("襲撃だ！", "You are ambushed !"));
            creature_ptr->oldpy = randint1(MAX_HGT - 2);
            creature_ptr->oldpx = randint1(MAX_WID - 2);
            change_wild_mode(creature_ptr, TRUE);
            take_turn(creature_ptr, 100);
        }
    }

    if (!more)
        disturb(creature_ptr, FALSE, FALSE);
}

/*!
 * @brief 「走る」動作コマンドのメインルーチン /
 * Start running.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_run(player_type *creature_ptr)
{
    DIRECTION dir;
    if (cmd_limit_confused(creature_ptr))
        return;

    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (get_rep_dir(creature_ptr, &dir, FALSE)) {
        creature_ptr->running = (command_arg ? command_arg : 1000);
        run_step(creature_ptr, dir);
    }
}

/*!
 * @brief 「留まる」動作コマンドのメインルーチン /
 * Stay still.  Search.  Enter stores.
 * Pick up treasure if "pickup" is true.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param pickup アイテムの自動拾いを行うならTRUE
 * @return なし
 */
void do_cmd_stay(player_type *creature_ptr, bool pickup)
{
    u32b mpe_mode = MPE_STAYING | MPE_ENERGY_USE;
    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= (PR_STATE);
        command_arg = 0;
    }

    take_turn(creature_ptr, 100);
    if (pickup)
        mpe_mode |= MPE_DO_PICKUP;

    (void)move_player_effect(creature_ptr, creature_ptr->y, creature_ptr->x, mpe_mode);
}

/*!
 * @brief 「休む」動作コマンドのメインルーチン /
 * Resting allows a player to safely restore his hp	-RAK-
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_rest(player_type *creature_ptr)
{
    set_action(creature_ptr, ACTION_NONE);
    if ((creature_ptr->pclass == CLASS_BARD) && (SINGING_SONG_EFFECT(creature_ptr) || INTERUPTING_SONG_EFFECT(creature_ptr)))
        stop_singing(creature_ptr);

    if (hex_spelling_any(creature_ptr))
        stop_hex_spell_all(creature_ptr);

    if (command_arg <= 0) {
        concptr p = _("休憩 (0-9999, '*' で HP/MP全快, '&' で必要なだけ): ", "Rest (0-9999, '*' for HP/SP, '&' as needed): ");
        char out_val[80];
        strcpy(out_val, "&");
        if (!get_string(p, out_val, 4))
            return;

        if (out_val[0] == '&') {
            command_arg = COMMAND_ARG_REST_UNTIL_DONE;
        } else if (out_val[0] == '*') {
            command_arg = COMMAND_ARG_REST_FULL_HEALING;
        } else {
            command_arg = (COMMAND_ARG)atoi(out_val);
            if (command_arg <= 0)
                return;
        }
    }

    if (command_arg > 9999)
        command_arg = 9999;

    if (creature_ptr->special_defense & NINJA_S_STEALTH)
        set_superstealth(creature_ptr, FALSE);

    take_turn(creature_ptr, 100);
    if (command_arg > 100)
        chg_virtue(creature_ptr, V_DILIGENCE, -1);

    if ((creature_ptr->chp == creature_ptr->mhp) && (creature_ptr->csp == creature_ptr->msp) && !creature_ptr->blind && !creature_ptr->confused
        && !creature_ptr->poisoned && !creature_ptr->afraid && !creature_ptr->stun && !creature_ptr->cut && !creature_ptr->slow && !creature_ptr->paralyzed
        && !creature_ptr->image && !creature_ptr->word_recall && !creature_ptr->alter_reality)
        chg_virtue(creature_ptr, V_DILIGENCE, -1);

    creature_ptr->resting = command_arg;
    creature_ptr->action = ACTION_REST;
    creature_ptr->update |= (PU_BONUS);
    update_creature(creature_ptr);
    creature_ptr->redraw |= (PR_STATE);
    update_output(creature_ptr);
    Term_fresh();
}

/*
 * todo Doxygenの加筆求む
 * @brief 射撃処理のメインルーチン
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param snipe_type ？？？
 * @return なし
 */
void do_cmd_fire(player_type *creature_ptr, SPELL_IDX snipe_type)
{
    OBJECT_IDX item;
    object_type *j_ptr, *ammo_ptr;
    if (creature_ptr->wild_mode)
        return;

    creature_ptr->is_fired = FALSE;
    j_ptr = &creature_ptr->inventory_list[INVEN_BOW];
    if (!j_ptr->tval) {
        msg_print(_("射撃用の武器を持っていない。", "You have nothing to fire with."));
        flush();
        return;
    }

    if (j_ptr->sval == SV_CRIMSON) {
        msg_print(_("この武器は発動して使うもののようだ。", "It's already activated."));
        flush();
        return;
    }

    if (j_ptr->sval == SV_HARP) {
        msg_print(_("この武器で射撃はできない。", "It's not for firing."));
        flush();
        return;
    }

    if (creature_ptr->special_defense & KATA_MUSOU) {
        set_action(creature_ptr, ACTION_NONE);
    }

    concptr q = _("どれを撃ちますか? ", "Fire which item? ");
    concptr s = _("発射されるアイテムがありません。", "You have nothing to fire.");
    ammo_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), creature_ptr->tval_ammo);
    if (!ammo_ptr) {
        flush();
        return;
    }

    exe_fire(creature_ptr, item, j_ptr, snipe_type);
    if (!creature_ptr->is_fired || creature_ptr->pclass != CLASS_SNIPER)
        return;

    if (snipe_type == SP_AWAY)
        teleport_player(creature_ptr, 10 + (creature_ptr->concent * 2), TELEPORT_SPONTANEOUS);

    if (snipe_type == SP_FINAL) {
        msg_print(_("射撃の反動が体を襲った。", "The weapon's recoil stuns you. "));
        (void)set_slow(creature_ptr, creature_ptr->slow + randint0(7) + 7, FALSE);
        (void)set_stun(creature_ptr, creature_ptr->stun + randint1(25));
    }
}

/*!
 * @brief 投射処理メインルーチン /
 * Throw an object from the pack or floor.
 * @param mult 威力の倍率
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param boomerang ブーメラン処理ならばTRUE
 * @param shuriken 忍者の手裏剣処理ならばTRUE
 * @return ターンを消費した場合TRUEを返す
 * @details
 * <pre>
 * Note: "unseen" monsters are very hard to hit.
 *
 * Should throwing a weapon do full damage?  Should it allow the magic
 * to hit bonus of the weapon to have an effect?  Should it ever cause
 * the item to be destroyed?  Should it do any damage at all?
 * </pre>
 */
bool do_cmd_throw(player_type *creature_ptr, int mult, bool boomerang, OBJECT_IDX shuriken)
{
    DIRECTION dir;
    OBJECT_IDX item;
    int i;
    POSITION y, x, ty, tx, prev_y, prev_x;
    POSITION ny[19], nx[19];
    int chance, tdam, tdis;
    int mul, div, dd, ds;
    int cur_dis, visible;
    PERCENTAGE j;
    object_type forge;
    object_type *q_ptr;
    object_type *o_ptr;
    bool hit_body = FALSE;
    bool hit_wall = FALSE;
    bool equiped_item = FALSE;
    bool return_when_thrown = FALSE;
    GAME_TEXT o_name[MAX_NLEN];
    int msec = delay_factor * delay_factor * delay_factor;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    concptr q, s;
    bool come_back = FALSE;
    bool do_drop = TRUE;
    if (creature_ptr->wild_mode)
        return FALSE;

    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (shuriken >= 0) {
        item = shuriken;
        o_ptr = &creature_ptr->inventory_list[item];
    } else if (boomerang) {
        if (has_melee_weapon(creature_ptr, INVEN_RARM) && has_melee_weapon(creature_ptr, INVEN_LARM)) {
            item_tester_hook = item_tester_hook_boomerang;
            q = _("どの武器を投げますか? ", "Throw which item? ");
            s = _("投げる武器がない。", "You have nothing to throw.");
            o_ptr = choose_object(creature_ptr, &item, q, s, (USE_EQUIP), 0);
            if (!o_ptr) {
                flush();
                return FALSE;
            }
        } else if (has_melee_weapon(creature_ptr, INVEN_LARM)) {
            item = INVEN_LARM;
            o_ptr = &creature_ptr->inventory_list[item];
        } else {
            item = INVEN_RARM;
            o_ptr = &creature_ptr->inventory_list[item];
        }
    } else {
        q = _("どのアイテムを投げますか? ", "Throw which item? ");
        s = _("投げるアイテムがない。", "You have nothing to throw.");
        o_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR | USE_EQUIP), 0);
        if (!o_ptr) {
            flush();
            return FALSE;
        }
    }

    if (object_is_cursed(o_ptr) && (item >= INVEN_RARM)) {
        msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
        return FALSE;
    }

    if (creature_ptr->current_floor_ptr->inside_arena && !boomerang && (o_ptr->tval != TV_SPIKE)) {
        msg_print(_("アリーナではアイテムを使えない！", "You're in the arena now. This is hand-to-hand!"));
        msg_print(NULL);
        return FALSE;
    }

    q_ptr = &forge;
    object_copy(q_ptr, o_ptr);
    object_flags(creature_ptr, q_ptr, flgs);
    torch_flags(q_ptr, flgs);
    distribute_charges(o_ptr, q_ptr, 1);
    q_ptr->number = 1;
    describe_flavor(creature_ptr, o_name, q_ptr, OD_OMIT_PREFIX);
    if (creature_ptr->mighty_throw)
        mult += 3;

    mul = 10 + 2 * (mult - 1);
    div = ((q_ptr->weight > 10) ? q_ptr->weight : 10);
    if ((have_flag(flgs, TR_THROW)) || boomerang)
        div /= 2;

    tdis = (adj_str_blow[creature_ptr->stat_ind[A_STR]] + 20) * mul / div;
    if (tdis > mul)
        tdis = mul;

    if (shuriken >= 0) {
        ty = randint0(101) - 50 + creature_ptr->y;
        tx = randint0(101) - 50 + creature_ptr->x;
    } else {
        project_length = tdis + 1;
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        tx = creature_ptr->x + 99 * ddx[dir];
        ty = creature_ptr->y + 99 * ddy[dir];
        if ((dir == 5) && target_okay(creature_ptr)) {
            tx = target_col;
            ty = target_row;
        }

        project_length = 0; /* reset to default */
    }

    if ((q_ptr->name1 == ART_MJOLLNIR) || (q_ptr->name1 == ART_AEGISFANG) || boomerang)
        return_when_thrown = TRUE;

    if (item >= 0) {
        inven_item_increase(creature_ptr, item, -1);
        if (!return_when_thrown)
            inven_item_describe(creature_ptr, item);
        inven_item_optimize(creature_ptr, item);
    } else {
        floor_item_increase(creature_ptr->current_floor_ptr, 0 - item, -1);
        floor_item_optimize(creature_ptr, 0 - item);
    }

    if (item >= INVEN_RARM) {
        equiped_item = TRUE;
        creature_ptr->redraw |= (PR_EQUIPPY);
    }

    take_turn(creature_ptr, 100);
    if ((creature_ptr->pclass == CLASS_ROGUE) || (creature_ptr->pclass == CLASS_NINJA))
        creature_ptr->energy_use -= creature_ptr->lev;

    y = creature_ptr->y;
    x = creature_ptr->x;
    handle_stuff(creature_ptr);
    if ((creature_ptr->pclass == CLASS_NINJA) && ((q_ptr->tval == TV_SPIKE) || ((have_flag(flgs, TR_THROW)) && (q_ptr->tval == TV_SWORD))))
        shuriken = TRUE;
    else
        shuriken = FALSE;

    if (have_flag(flgs, TR_THROW))
        chance = ((creature_ptr->skill_tht) + ((creature_ptr->to_h_b + q_ptr->to_h) * BTH_PLUS_ADJ));
    else
        chance = (creature_ptr->skill_tht + (creature_ptr->to_h_b * BTH_PLUS_ADJ));

    if (shuriken)
        chance *= 2;

    prev_y = y;
    prev_x = x;
    for (cur_dis = 0; cur_dis <= tdis;) {
        if ((y == ty) && (x == tx))
            break;

        ny[cur_dis] = y;
        nx[cur_dis] = x;
        mmove2(&ny[cur_dis], &nx[cur_dis], creature_ptr->y, creature_ptr->x, ty, tx);
        if (!cave_have_flag_bold(creature_ptr->current_floor_ptr, ny[cur_dis], nx[cur_dis], FF_PROJECT)) {
            hit_wall = TRUE;
            if ((q_ptr->tval == TV_FIGURINE) || object_is_potion(q_ptr) || !creature_ptr->current_floor_ptr->grid_array[ny[cur_dis]][nx[cur_dis]].m_idx)
                break;
        }

        if (panel_contains(ny[cur_dis], nx[cur_dis]) && player_can_see_bold(creature_ptr, ny[cur_dis], nx[cur_dis])) {
            SYMBOL_CODE c = object_char(q_ptr);
            TERM_COLOR a = object_attr(q_ptr);
            print_rel(creature_ptr, c, a, ny[cur_dis], nx[cur_dis]);
            move_cursor_relative(ny[cur_dis], nx[cur_dis]);
            Term_fresh();
            Term_xtra(TERM_XTRA_DELAY, msec);
            lite_spot(creature_ptr, ny[cur_dis], nx[cur_dis]);
            Term_fresh();
        } else {
            Term_xtra(TERM_XTRA_DELAY, msec);
        }

        prev_y = y;
        prev_x = x;
        x = nx[cur_dis];
        y = ny[cur_dis];
        cur_dis++;
        if (creature_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
            grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
            monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
            GAME_TEXT m_name[MAX_NLEN];
            monster_name(creature_ptr, g_ptr->m_idx, m_name);
            visible = m_ptr->ml;
            hit_body = TRUE;
            if (test_hit_fire(creature_ptr, chance - cur_dis, m_ptr, m_ptr->ml, o_name)) {
                bool fear = FALSE;
                if (!visible) {
                    msg_format(_("%sが敵を捕捉した。", "The %s finds a mark."), o_name);
                } else {
                    msg_format(_("%sが%sに命中した。", "The %s hits %s."), o_name, m_name);
                    if (m_ptr->ml) {
                        if (!creature_ptr->image)
                            monster_race_track(creature_ptr, m_ptr->ap_r_idx);
                        health_track(creature_ptr, g_ptr->m_idx);
                    }
                }

                dd = q_ptr->dd;
                ds = q_ptr->ds;
                torch_dice(q_ptr, &dd, &ds);
                tdam = damroll(dd, ds);
                tdam = calc_attack_damage_with_slay(creature_ptr, q_ptr, tdam, m_ptr, 0, TRUE);
                tdam = critical_shot(creature_ptr, q_ptr->weight, q_ptr->to_h, 0, tdam);
                if (q_ptr->to_d > 0)
                    tdam += q_ptr->to_d;
                else
                    tdam += -q_ptr->to_d;

                if (boomerang) {
                    tdam *= (mult + creature_ptr->num_blow[item - INVEN_RARM]);
                    tdam += creature_ptr->to_d_m;
                } else if (have_flag(flgs, TR_THROW)) {
                    tdam *= (3 + mult);
                    tdam += creature_ptr->to_d_m;
                } else {
                    tdam *= mult;
                }

                if (shuriken)
                    tdam += ((creature_ptr->lev + 30) * (creature_ptr->lev + 30) - 900) / 55;

                if (tdam < 0)
                    tdam = 0;

                tdam = mon_damage_mod(creature_ptr, m_ptr, tdam, FALSE);
                msg_format_wizard(creature_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), tdam,
                    m_ptr->hp - tdam, m_ptr->maxhp, m_ptr->max_maxhp);

                if (mon_take_hit(creature_ptr, g_ptr->m_idx, tdam, &fear, extract_note_dies(real_r_idx(m_ptr)))) {
                    /* Dead monster */
                } else {
                    message_pain(creature_ptr, g_ptr->m_idx, tdam);
                    if ((tdam > 0) && !object_is_potion(q_ptr))
                        anger_monster(creature_ptr, m_ptr);

                    if (fear && m_ptr->ml) {
                        sound(SOUND_FLEE);
                        msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), m_name);
                    }
                }
            }

            break;
        }
    }

    if (hit_body)
        torch_lost_fuel(q_ptr);

    j = (hit_body ? breakage_chance(creature_ptr, q_ptr, creature_ptr->pclass == CLASS_ARCHER, 0) : 0);

    if ((q_ptr->tval == TV_FIGURINE) && !(creature_ptr->current_floor_ptr->inside_arena)) {
        j = 100;
        if (!(summon_named_creature(creature_ptr, 0, y, x, q_ptr->pval, !(object_is_cursed(q_ptr)) ? PM_FORCE_PET : 0L)))
            msg_print(_("人形は捻じ曲がり砕け散ってしまった！", "The Figurine writhes and then shatters."));
        else if (object_is_cursed(q_ptr))
            msg_print(_("これはあまり良くない気がする。", "You have a bad feeling about this."));
    }

    if (object_is_potion(q_ptr)) {
        if (hit_body || hit_wall || (randint1(100) < j)) {
            msg_format(_("%sは砕け散った！", "The %s shatters!"), o_name);
            if (potion_smash_effect(creature_ptr, 0, y, x, q_ptr->k_idx)) {
                monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[y][x].m_idx];
                if (creature_ptr->current_floor_ptr->grid_array[y][x].m_idx && is_friendly(m_ptr) && !monster_invulner_remaining(m_ptr)) {
                    GAME_TEXT m_name[MAX_NLEN];
                    monster_desc(creature_ptr, m_name, m_ptr, 0);
                    msg_format(_("%sは怒った！", "%^s gets angry!"), m_name);
                    set_hostile(creature_ptr, &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[y][x].m_idx]);
                }
            }

            do_drop = FALSE;
        } else {
            j = 0;
        }
    }

    if (return_when_thrown) {
        int back_chance = randint1(30) + 20 + ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
        char o2_name[MAX_NLEN];
        bool super_boomerang = (((q_ptr->name1 == ART_MJOLLNIR) || (q_ptr->name1 == ART_AEGISFANG)) && boomerang);
        j = -1;
        if (boomerang)
            back_chance += 4 + randint1(5);
        if (super_boomerang)
            back_chance += 100;
        describe_flavor(creature_ptr, o2_name, q_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        if ((back_chance > 30) && (!one_in_(100) || super_boomerang)) {
            for (i = cur_dis - 1; i > 0; i--) {
                if (panel_contains(ny[i], nx[i]) && player_can_see_bold(creature_ptr, ny[i], nx[i])) {
                    SYMBOL_CODE c = object_char(q_ptr);
                    byte a = object_attr(q_ptr);
                    print_rel(creature_ptr, c, a, ny[i], nx[i]);
                    move_cursor_relative(ny[i], nx[i]);
                    Term_fresh();
                    Term_xtra(TERM_XTRA_DELAY, msec);
                    lite_spot(creature_ptr, ny[i], nx[i]);
                    Term_fresh();
                } else {
                    Term_xtra(TERM_XTRA_DELAY, msec);
                }
            }

            if ((back_chance > 37) && !creature_ptr->blind && (item >= 0)) {
                msg_format(_("%sが手元に返ってきた。", "%s comes back to you."), o2_name);
                come_back = TRUE;
            } else {
                if (item >= 0) {
                    msg_format(_("%sを受け損ねた！", "%s comes back, but you can't catch!"), o2_name);
                } else {
                    msg_format(_("%sが返ってきた。", "%s comes back."), o2_name);
                }
                y = creature_ptr->y;
                x = creature_ptr->x;
            }
        } else {
            msg_format(_("%sが返ってこなかった！", "%s doesn't come back!"), o2_name);
        }
    }

    if (come_back) {
        if (item == INVEN_RARM || item == INVEN_LARM) {
            o_ptr = &creature_ptr->inventory_list[item];
            object_copy(o_ptr, q_ptr);
            creature_ptr->total_weight += q_ptr->weight;
            creature_ptr->equip_cnt++;
            creature_ptr->update |= (PU_BONUS | PU_TORCH | PU_MANA);
            creature_ptr->window |= (PW_EQUIP);
        } else {
            store_item_to_inventory(creature_ptr, q_ptr);
        }

        do_drop = FALSE;
    } else if (equiped_item) {
        verify_equip_slot(creature_ptr, item);
        calc_android_exp(creature_ptr);
    }

    if (do_drop) {
        if (cave_have_flag_bold(creature_ptr->current_floor_ptr, y, x, FF_PROJECT)) {
            (void)drop_near(creature_ptr, q_ptr, j, y, x);
        } else {
            (void)drop_near(creature_ptr, q_ptr, j, prev_y, prev_x);
        }
    }

    return TRUE;
}

/*!
 * @brief 自殺するコマンドのメインルーチン
 * commit suicide
 * @return なし
 * @details
 */
void do_cmd_suicide(player_type *creature_ptr)
{
    flush();
    if (current_world_ptr->total_winner) {
        if (!get_check_strict(creature_ptr, _("引退しますか? ", "Do you want to retire? "), CHECK_NO_HISTORY))
            return;
    } else {
        if (!get_check(_("本当に自殺しますか？", "Do you really want to commit suicide? ")))
            return;
    }

    if (!current_world_ptr->noscore) {
        prt(_("確認のため '@' を押して下さい。", "Please verify SUICIDE by typing the '@' sign: "), 0, 0);
        flush();
        int i = inkey();
        prt("", 0, 0);
        if (i != '@')
            return;

        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_GAMEOVER);
    }

    if (creature_ptr->last_message)
        string_free(creature_ptr->last_message);

    creature_ptr->last_message = NULL;
    if (current_world_ptr->total_winner && last_words) {
        char buf[1024] = "";
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_WINNER);
        do {
            while (!get_string(_("*勝利*メッセージ: ", "*Winning* message: "), buf, sizeof buf))
                ;
        } while (!get_check_strict(creature_ptr, _("よろしいですか？", "Are you sure? "), CHECK_NO_HISTORY));

        if (buf[0]) {
            creature_ptr->last_message = string_make(buf);
            msg_print(creature_ptr->last_message);
        }
    }

    creature_ptr->playing = FALSE;
    creature_ptr->is_dead = TRUE;
    creature_ptr->leaving = TRUE;
    if (!current_world_ptr->total_winner) {
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 0, _("ダンジョンの探索に絶望して自殺した。", "gave up all hope to commit suicide."));
        exe_write_diary(creature_ptr, DIARY_GAMESTART, 1, _("-------- ゲームオーバー --------", "--------   Game  Over   --------"));
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, "\n\n\n\n");
    }

    (void)strcpy(creature_ptr->died_from, _("途中終了", "Quitting"));
}
