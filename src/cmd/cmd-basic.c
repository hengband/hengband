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
#include "action/movement-execution.h"
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

static bool exe_alter(player_type *creature_ptr)
{
    DIRECTION dir;
    if (!get_rep_dir(creature_ptr, &dir, TRUE))
        return FALSE;

    POSITION y = creature_ptr->y + ddy[dir];
    POSITION x = creature_ptr->x + ddx[dir];
    grid_type *g_ptr;
    g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    FEAT_IDX feat = get_feat_mimic(g_ptr);
    feature_type *f_ptr;
    f_ptr = &f_info[feat];
    take_turn(creature_ptr, 100);
    if (g_ptr->m_idx) {
        do_cmd_attack(creature_ptr, y, x, 0);
        return FALSE;
    }
    
    if (have_flag(f_ptr->flags, FF_OPEN))
        return exe_open(creature_ptr, y, x);
    
    if (have_flag(f_ptr->flags, FF_BASH))
        return exe_bash(creature_ptr, y, x, dir);
    
    if (have_flag(f_ptr->flags, FF_TUNNEL))
        return exe_tunnel(creature_ptr, y, x);
    
    if (have_flag(f_ptr->flags, FF_CLOSE))
        return exe_close(creature_ptr, y, x);
    
    if (have_flag(f_ptr->flags, FF_DISARM))
        return exe_disarm(creature_ptr, y, x, dir);

    msg_print(_("何もない空中を攻撃した。", "You attack the empty air."));
    return FALSE;
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
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= (PR_STATE);
        command_arg = 0;
    }

    if (!exe_alter(creature_ptr))
        disturb(creature_ptr, FALSE, FALSE);
}

/*!
 * @brief 自殺/引退/切腹の確認
 * @param なし
 * @return 自殺/引退/切腹を実施するならTRUE、キャンセルならFALSE
 */
static bool decide_suicide(void)
{
    if (current_world_ptr->noscore)
        return TRUE;

    prt(_("確認のため '@' を押して下さい。", "Please verify SUICIDE by typing the '@' sign: "), 0, 0);
    flush();
    int i = inkey();
    prt("", 0, 0);
    return i == '@';
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

    if (!decide_suicide())
        return;

    if (creature_ptr->last_message)
        string_free(creature_ptr->last_message);

    creature_ptr->last_message = NULL;
    if (current_world_ptr->total_winner && last_words) {
        char buf[1024] = "";
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_WINNER);
        do {
            while (!get_string(_("*勝利*メッセージ: ", "*Winning* message: "), buf, sizeof(buf)))
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
