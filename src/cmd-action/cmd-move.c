#include "cmd-action/cmd-move.h"
#include "action/action-limited.h"
#include "action/movement-execution.h"
#include "action/run-execution.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/output-updater.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-mode-changer.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "mind/mind-ninja.h"
#include "player/attack-defense-types.h"
#include "player-info/avatar.h"
#include "player/player-move.h"
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief フロア脱出時に出戻りが不可能だった場合に警告を加える処理
 * @param down_stair TRUEならば階段を降りる処理、FALSEなら階段を昇る処理による内容
 * @return フロア移動を実際に行うならTRUE、キャンセルする場合はFALSE
 */
static bool confirm_leave_level(player_type *creature_ptr, bool down_stair)
{
    quest_type *q_ptr = &quest[creature_ptr->current_floor_ptr->inside_quest];
    if (confirm_quest && creature_ptr->current_floor_ptr->inside_quest
        && (q_ptr->type == QUEST_TYPE_RANDOM || (q_ptr->flags & QUEST_FLAG_ONCE && q_ptr->status != QUEST_STATUS_COMPLETED)
            || (q_ptr->flags & QUEST_FLAG_TOWER
                && ((q_ptr->status != QUEST_STATUS_STAGE_COMPLETED) || (down_stair && (quest[QUEST_TOWER1].status != QUEST_STATUS_COMPLETED)))))) {
        msg_print(_("この階を一度去ると二度と戻って来られません。", "You can't come back here once you leave this floor."));
        return get_check(_("本当にこの階を去りますか？", "Really leave this floor? "));
    }

    return TRUE;
}

/*!
 * @brief 階段を使って階層を昇る処理 / Go up one level
 * @return なし
 */
void do_cmd_go_up(player_type *creature_ptr)
{
    bool go_up = FALSE;
    grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    int up_num = 0;
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (!has_flag(f_ptr->flags, FF_LESS)) {
        msg_print(_("ここには上り階段が見当たらない。", "I see no up staircase here."));
        return;
    }

    if (has_flag(f_ptr->flags, FF_QUEST)) {
        if (!confirm_leave_level(creature_ptr, FALSE))
            return;

        if (is_echizen(creature_ptr))
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        else
            msg_print(_("上の階に登った。", "You enter the up staircase."));

        leave_quest_check(creature_ptr);
        creature_ptr->current_floor_ptr->inside_quest = g_ptr->special;
        if (!quest[creature_ptr->current_floor_ptr->inside_quest].status) {
            if (quest[creature_ptr->current_floor_ptr->inside_quest].type != QUEST_TYPE_RANDOM) {
                init_flags = INIT_ASSIGN;
                parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
            }

            quest[creature_ptr->current_floor_ptr->inside_quest].status = QUEST_STATUS_TAKEN;
        }

        if (!creature_ptr->current_floor_ptr->inside_quest)
            creature_ptr->current_floor_ptr->dun_level = 0;

        creature_ptr->leaving = TRUE;
        creature_ptr->oldpx = 0;
        creature_ptr->oldpy = 0;
        take_turn(creature_ptr, 100);
        return;
    }

    if (!creature_ptr->current_floor_ptr->dun_level)
        go_up = TRUE;
    else
        go_up = confirm_leave_level(creature_ptr, FALSE);

    if (!go_up)
        return;

    take_turn(creature_ptr, 100);

    if (autosave_l)
        do_cmd_save_game(creature_ptr, TRUE);

    if (creature_ptr->current_floor_ptr->inside_quest && quest[creature_ptr->current_floor_ptr->inside_quest].type == QUEST_TYPE_RANDOM) {
        leave_quest_check(creature_ptr);
        creature_ptr->current_floor_ptr->inside_quest = 0;
    }

    if (creature_ptr->current_floor_ptr->inside_quest && quest[creature_ptr->current_floor_ptr->inside_quest].type != QUEST_TYPE_RANDOM) {
        leave_quest_check(creature_ptr);
        creature_ptr->current_floor_ptr->inside_quest = g_ptr->special;
        creature_ptr->current_floor_ptr->dun_level = 0;
        up_num = 0;
    } else {
        if (has_flag(f_ptr->flags, FF_SHAFT)) {
            prepare_change_floor_mode(creature_ptr, CFM_SAVE_FLOORS | CFM_UP | CFM_SHAFT);
            up_num = 2;
        } else {
            prepare_change_floor_mode(creature_ptr, CFM_SAVE_FLOORS | CFM_UP);
            up_num = 1;
        }

        if (creature_ptr->current_floor_ptr->dun_level - up_num < d_info[creature_ptr->dungeon_idx].mindepth)
            up_num = creature_ptr->current_floor_ptr->dun_level;
    }

    if (record_stair)
        exe_write_diary(creature_ptr, DIARY_STAIR, 0 - up_num, _("階段を上った", "climbed up the stairs to"));

    if (is_echizen(creature_ptr))
        msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
    else if (up_num == creature_ptr->current_floor_ptr->dun_level)
        msg_print(_("地上に戻った。", "You go back to the surface."));
    else
        msg_print(_("階段を上って新たなる迷宮へと足を踏み入れた。", "You enter a maze of up staircases."));

    creature_ptr->leaving = TRUE;
}

/*!
 * @brief 階段を使って階層を降りる処理 / Go down one level
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_go_down(player_type *creature_ptr)
{
    bool fall_trap = FALSE;
    int down_num = 0;
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    if (!has_flag(f_ptr->flags, FF_MORE)) {
        msg_print(_("ここには下り階段が見当たらない。", "I see no down staircase here."));
        return;
    }

    if (has_flag(f_ptr->flags, FF_TRAP))
        fall_trap = TRUE;

    if (has_flag(f_ptr->flags, FF_QUEST_ENTER)) {
        do_cmd_quest(creature_ptr);
        return;
    }

    if (has_flag(f_ptr->flags, FF_QUEST)) {
        if (!confirm_leave_level(creature_ptr, TRUE))
            return;

        if (is_echizen(creature_ptr))
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        else
            msg_print(_("下の階に降りた。", "You enter the down staircase."));

        leave_quest_check(creature_ptr);
        leave_tower_check(creature_ptr);
        creature_ptr->current_floor_ptr->inside_quest = g_ptr->special;
        if (!quest[creature_ptr->current_floor_ptr->inside_quest].status) {
            if (quest[creature_ptr->current_floor_ptr->inside_quest].type != QUEST_TYPE_RANDOM) {
                init_flags = INIT_ASSIGN;
                parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
            }

            quest[creature_ptr->current_floor_ptr->inside_quest].status = QUEST_STATUS_TAKEN;
        }

        if (!creature_ptr->current_floor_ptr->inside_quest)
            creature_ptr->current_floor_ptr->dun_level = 0;

        creature_ptr->leaving = TRUE;
        creature_ptr->oldpx = 0;
        creature_ptr->oldpy = 0;
        take_turn(creature_ptr, 100);
        return;
    }

    DUNGEON_IDX target_dungeon = 0;
    if (!creature_ptr->current_floor_ptr->dun_level) {
        target_dungeon = has_flag(f_ptr->flags, FF_ENTRANCE) ? g_ptr->special : DUNGEON_ANGBAND;
        if (ironman_downward && (target_dungeon != DUNGEON_ANGBAND)) {
            msg_print(_("ダンジョンの入口は塞がれている！", "The entrance of this dungeon is closed!"));
            return;
        }

        if (!max_dlv[target_dungeon]) {
            msg_format(_("ここには%sの入り口(%d階相当)があります", "There is the entrance of %s (Danger level: %d)"), d_name + d_info[target_dungeon].name,
                d_info[target_dungeon].mindepth);
            if (!get_check(_("本当にこのダンジョンに入りますか？", "Do you really get in this dungeon? ")))
                return;
        }

        creature_ptr->oldpx = creature_ptr->x;
        creature_ptr->oldpy = creature_ptr->y;
        creature_ptr->dungeon_idx = target_dungeon;
        prepare_change_floor_mode(creature_ptr, CFM_FIRST_FLOOR);
    }

    take_turn(creature_ptr, 100);
    if (autosave_l)
        do_cmd_save_game(creature_ptr, TRUE);

    if (has_flag(f_ptr->flags, FF_SHAFT))
        down_num += 2;
    else
        down_num += 1;

    if (!creature_ptr->current_floor_ptr->dun_level) {
        creature_ptr->enter_dungeon = TRUE;
        down_num = d_info[creature_ptr->dungeon_idx].mindepth;
    }

    if (record_stair) {
        if (fall_trap)
            exe_write_diary(creature_ptr, DIARY_STAIR, down_num, _("落とし戸に落ちた", "fell through a trap door"));
        else
            exe_write_diary(creature_ptr, DIARY_STAIR, down_num, _("階段を下りた", "climbed down the stairs to"));
    }

    if (fall_trap) {
        msg_print(_("わざと落とし戸に落ちた。", "You deliberately jump through the trap door."));
    } else {
        if (target_dungeon) {
            msg_format(_("%sへ入った。", "You entered %s."), d_text + d_info[creature_ptr->dungeon_idx].text);
        } else {
            if (is_echizen(creature_ptr))
                msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
            else
                msg_print(_("階段を下りて新たなる迷宮へと足を踏み入れた。", "You enter a maze of down staircases."));
        }
    }

    creature_ptr->leaving = TRUE;

    if (fall_trap) {
        prepare_change_floor_mode(creature_ptr, CFM_SAVE_FLOORS | CFM_DOWN | CFM_RAND_PLACE | CFM_RAND_CONNECT);
        return;
    }

    if (has_flag(f_ptr->flags, FF_SHAFT))
        prepare_change_floor_mode(creature_ptr, CFM_SAVE_FLOORS | CFM_DOWN | CFM_SHAFT);
    else
        prepare_change_floor_mode(creature_ptr, CFM_SAVE_FLOORS | CFM_DOWN);
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
        creature_ptr->redraw |= PR_STATE;
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

        exe_movement(creature_ptr, dir, pickup, FALSE);
        more = TRUE;
    }

    if (creature_ptr->wild_mode && !cave_has_flag_bold(creature_ptr->current_floor_ptr, creature_ptr->y, creature_ptr->x, FF_TOWN)) {
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
    creature_ptr->update |= PU_BONUS;
    update_creature(creature_ptr);
    creature_ptr->redraw |= (PR_STATE);
    update_output(creature_ptr);
    term_fresh();
}
