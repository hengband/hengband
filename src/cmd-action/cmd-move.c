#include "cmd-action/cmd-move.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "io/write-diary.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "system/system-variables.h"
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

    if (!have_flag(f_ptr->flags, FF_LESS)) {
        msg_print(_("ここには上り階段が見当たらない。", "I see no up staircase here."));
        return;
    }

    if (have_flag(f_ptr->flags, FF_QUEST)) {
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
        if (have_flag(f_ptr->flags, FF_SHAFT)) {
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
    if (!have_flag(f_ptr->flags, FF_MORE)) {
        msg_print(_("ここには下り階段が見当たらない。", "I see no down staircase here."));
        return;
    }

    if (have_flag(f_ptr->flags, FF_TRAP))
        fall_trap = TRUE;

    if (have_flag(f_ptr->flags, FF_QUEST_ENTER)) {
        do_cmd_quest(creature_ptr);
        return;
    }

    if (have_flag(f_ptr->flags, FF_QUEST)) {
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
        target_dungeon = have_flag(f_ptr->flags, FF_ENTRANCE) ? g_ptr->special : DUNGEON_ANGBAND;
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

    if (have_flag(f_ptr->flags, FF_SHAFT))
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

    if (have_flag(f_ptr->flags, FF_SHAFT))
        prepare_change_floor_mode(creature_ptr, CFM_SAVE_FLOORS | CFM_DOWN | CFM_SHAFT);
    else
        prepare_change_floor_mode(creature_ptr, CFM_SAVE_FLOORS | CFM_DOWN);
}
