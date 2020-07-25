/*!
 * @brief 雑多なその他の処理2 / effects of various "objects"
 * @date 2014/02/06
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "target/targeting.h"
#include "action/travel-execution.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-building/cmd-building.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/spells-effect-util.h"
#include "flavor/flavor-describer.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-object.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "floor/object-scanner.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/keymap-directory-getter.h"
#include "game-option/map-screen-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "io/command-repeater.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/screen-util.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/smart-learn-types.h"
#include "object-enchant/object-curse.h"
#include "object/object-kind-hook.h"
#include "object/object-mark-types.h"
#include "player/player-race-types.h"
#include "player/player-status.h"
#include "spell/spells-summon.h"
#include "system/building-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "target/target-describer.h"
#include "target/target-preparation.h"
#include "target/target-setter.h"
#include "target/target-types.h" // todo 相互依存.
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
#include "view/display-lore.h"
#include "view/display-messages.h"
#include "view/display-monster-status.h"
#include "window/main-window-util.h"
#include "world/world.h"

/*!
 * @brief マップ描画のフォーカスを当てるべき座標を更新する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @details
 * Given an row (y) and col (x), this routine detects when a move
 * off the screen has occurred and figures new borders. -RAK-
 * "Update" forces a "full update" to take place.
 * The map is reprinted if necessary, and "TRUE" is returned.
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
void verify_panel(player_type *creature_ptr)
{
    POSITION y = creature_ptr->y;
    POSITION x = creature_ptr->x;
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    int max_prow_min = creature_ptr->current_floor_ptr->height - hgt;
    int max_pcol_min = creature_ptr->current_floor_ptr->width - wid;
    if (max_prow_min < 0)
        max_prow_min = 0;
    if (max_pcol_min < 0)
        max_pcol_min = 0;

    int prow_min;
    int pcol_min;
    if (center_player && (center_running || !creature_ptr->running)) {
        prow_min = y - hgt / 2;
        if (prow_min < 0)
            prow_min = 0;
        else if (prow_min > max_prow_min)
            prow_min = max_prow_min;

        pcol_min = x - wid / 2;
        if (pcol_min < 0)
            pcol_min = 0;
        else if (pcol_min > max_pcol_min)
            pcol_min = max_pcol_min;
    } else {
        prow_min = panel_row_min;
        pcol_min = panel_col_min;
        if (y > panel_row_max - 2)
            while (y > prow_min + hgt - 1 - 2)
                prow_min += (hgt / 2);

        if (y < panel_row_min + 2)
            while (y < prow_min + 2)
                prow_min -= (hgt / 2);

        if (prow_min > max_prow_min)
            prow_min = max_prow_min;

        if (prow_min < 0)
            prow_min = 0;

        if (x > panel_col_max - 4)
            while (x > pcol_min + wid - 1 - 4)
                pcol_min += (wid / 2);

        if (x < panel_col_min + 4)
            while (x < pcol_min + 4)
                pcol_min -= (wid / 2);

        if (pcol_min > max_pcol_min)
            pcol_min = max_pcol_min;

        if (pcol_min < 0)
            pcol_min = 0;
    }

    if ((prow_min == panel_row_min) && (pcol_min == panel_col_min))
        return;

    panel_row_min = prow_min;
    panel_col_min = pcol_min;
    if (disturb_panel && !center_player)
        disturb(creature_ptr, FALSE, FALSE);

    panel_bounds_center();
    creature_ptr->update |= (PU_MONSTERS);
    creature_ptr->redraw |= (PR_MAP);
    creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}

/* Targetting variables */
MONSTER_IDX target_who;
POSITION target_col;
POSITION target_row;

/*
 * Update (if necessary) and verify (if possible) the target.
 * We return TRUE if the target is "okay" and FALSE otherwise.
 */
bool target_okay(player_type *creature_ptr)
{
    if (target_who < 0)
        return TRUE;

    if (target_who <= 0)
        return FALSE;

    if (!target_able(creature_ptr, target_who))
        return FALSE;

    monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[target_who];
    target_row = m_ptr->fy;
    target_col = m_ptr->fx;
    return TRUE;
}

/*
 * Get an "aiming direction" from the user.
 *
 * The "dir" is loaded with 1,2,3,4,6,7,8,9 for "actual direction", and
 * "0" for "current target", and "-1" for "entry aborted".
 *
 * Note that "Force Target", if set, will pre-empt user interaction,
 * if there is a usable target already set.
 *
 * Note that confusion over-rides any (explicit?) user choice.
 */
bool get_aim_dir(player_type *creature_ptr, DIRECTION *dp)
{
    DIRECTION dir = command_dir;
    if (use_old_target && target_okay(creature_ptr))
        dir = 5;

    COMMAND_CODE code;
    if (repeat_pull(&code))
        if (!(code == 5 && !target_okay(creature_ptr)))
            dir = (DIRECTION)code;

    *dp = (DIRECTION)code;
    char command;
    while (!dir) {
        concptr p;
        if (!target_okay(creature_ptr))
            p = _("方向 ('*'でターゲット選択, ESCで中断)? ", "Direction ('*' to choose a target, Escape to cancel)? ");
        else
            p = _("方向 ('5'でターゲットへ, '*'でターゲット再選択, ESCで中断)? ", "Direction ('5' for target, '*' to re-target, Escape to cancel)? ");

        if (!get_com(p, &command, TRUE))
            break;

        if (use_menu && (command == '\r'))
            command = 't';

        switch (command) {
        case 'T':
        case 't':
        case '.':
        case '5':
        case '0':
            dir = 5;
            break;
        case '*':
        case ' ':
        case '\r':
            if (target_set(creature_ptr, TARGET_KILL))
                dir = 5;

            break;
        default:
            dir = get_keymap_dir(command);
            break;
        }

        if ((dir == 5) && !target_okay(creature_ptr))
            dir = 0;

        if (!dir)
            bell();
    }

    if (!dir) {
        project_length = 0;
        return FALSE;
    }

    command_dir = dir;
    if (creature_ptr->confused)
        dir = ddd[randint0(8)];

    if (command_dir != dir)
        msg_print(_("あなたは混乱している。", "You are confused."));

    *dp = dir;
    repeat_push((COMMAND_CODE)command_dir);
    return TRUE;
}

bool get_direction(player_type *creature_ptr, DIRECTION *dp, bool allow_under, bool with_steed)
{
    DIRECTION dir = command_dir;
    COMMAND_CODE code;
    if (repeat_pull(&code))
        dir = (DIRECTION)code;

    *dp = (DIRECTION)code;
    concptr prompt = allow_under ? _("方向 ('.'足元, ESCで中断)? ", "Direction ('.' at feet, Escape to cancel)? ")
                                 : _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");

    while (!dir) {
        char ch;
        if (!get_com(prompt, &ch, TRUE))
            break;

        if ((allow_under) && ((ch == '5') || (ch == '-') || (ch == '.'))) {
            dir = 5;
            continue;
        }

        dir = get_keymap_dir(ch);
        if (!dir)
            bell();
    }

    if ((dir == 5) && (!allow_under))
        dir = 0;

    if (!dir)
        return FALSE;

    command_dir = dir;
    if (creature_ptr->confused) {
        if (randint0(100) < 75) {
            dir = ddd[randint0(8)];
        }
    } else if (creature_ptr->riding && with_steed) {
        monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (monster_confused_remaining(m_ptr)) {
            if (randint0(100) < 75)
                dir = ddd[randint0(8)];
        } else if ((r_ptr->flags1 & RF1_RAND_50) && (r_ptr->flags1 & RF1_RAND_25) && (randint0(100) < 50))
            dir = ddd[randint0(8)];
        else if ((r_ptr->flags1 & RF1_RAND_50) && (randint0(100) < 25))
            dir = ddd[randint0(8)];
    }

    if (command_dir != dir) {
        if (creature_ptr->confused) {
            msg_print(_("あなたは混乱している。", "You are confused."));
        } else {
            GAME_TEXT m_name[MAX_NLEN];
            monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];

            monster_desc(creature_ptr, m_name, m_ptr, 0);
            if (monster_confused_remaining(m_ptr)) {
                msg_format(_("%sは混乱している。", "%^s is confused."), m_name);
            } else {
                msg_format(_("%sは思い通りに動いてくれない。", "You cannot control %s."), m_name);
            }
        }
    }

    *dp = dir;
    repeat_push((COMMAND_CODE)command_dir);
    return TRUE;
}

/*
 * @brief 進行方向を指定する(騎乗対象の混乱の影響を受ける) / Request a "movement" direction (1,2,3,4,6,7,8,9) from the user,
 * and place it into "command_dir", unless we already have one.
 *
 * This function should be used for all "repeatable" commands, such as
 * run, walk, open, close, bash, disarm, spike, tunnel, etc, as well
 * as all commands which must reference a grid adjacent to the player,
 * and which may not reference the grid under the player.  Note that,
 * for example, it is no longer possible to "disarm" or "open" chests
 * in the same grid as the player.
 *
 * Direction "5" is illegal and will (cleanly) abort the command.
 *
 * This function tracks and uses the "global direction", and uses
 * that as the "desired direction", to which "confusion" is applied.
 */
bool get_rep_dir(player_type *creature_ptr, DIRECTION *dp, bool under)
{
    DIRECTION dir = command_dir;
    COMMAND_CODE code;
    if (repeat_pull(&code))
        dir = (DIRECTION)code;

    *dp = (DIRECTION)code;
    concptr prompt
        = under ? _("方向 ('.'足元, ESCで中断)? ", "Direction ('.' at feet, Escape to cancel)? ") : _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");
    while (!dir) {
        char ch;
        if (!get_com(prompt, &ch, TRUE))
            break;

        if ((under) && ((ch == '5') || (ch == '-') || (ch == '.'))) {
            dir = 5;
            continue;
        }

        dir = get_keymap_dir(ch);
        if (!dir)
            bell();
    }

    if ((dir == 5) && (!under))
        dir = 0;

    if (!dir)
        return FALSE;

    command_dir = dir;
    if (creature_ptr->confused) {
        if (randint0(100) < 75)
            dir = ddd[randint0(8)];
    } else if (creature_ptr->riding) {
        monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (monster_confused_remaining(m_ptr)) {
            if (randint0(100) < 75)
                dir = ddd[randint0(8)];
        } else if ((r_ptr->flags1 & RF1_RAND_50) && (r_ptr->flags1 & RF1_RAND_25) && (randint0(100) < 50))
            dir = ddd[randint0(8)];
        else if ((r_ptr->flags1 & RF1_RAND_50) && (randint0(100) < 25))
            dir = ddd[randint0(8)];
    }

    if (command_dir != dir) {
        if (creature_ptr->confused) {
            msg_print(_("あなたは混乱している。", "You are confused."));
        } else {
            GAME_TEXT m_name[MAX_NLEN];
            monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
            monster_desc(creature_ptr, m_name, m_ptr, 0);
            if (monster_confused_remaining(m_ptr))
                msg_format(_("%sは混乱している。", "%^s is confused."), m_name);
            else
                msg_format(_("%sは思い通りに動いてくれない。", "You cannot control %s."), m_name);
        }
    }

    *dp = dir;
    repeat_push((COMMAND_CODE)command_dir);
    return TRUE;
}
