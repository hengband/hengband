/*!
 * @brief その他の小さなコマンド処理群 (探索、汎用グリッド処理、自殺/引退/切腹)
 * @date 2014/01/02
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "cmd-action/cmd-others.h"
#include "action/open-close-execution.h"
#include "action/tunnel-execution.h"
#include "cmd-action/cmd-attack.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "floor/geometry.h"
#include "game-option/game-play-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-move.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 探索コマンドのメインルーチン / Simple command to "search" for one turn
 */
void do_cmd_search(player_type *creature_ptr)
{
    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= PR_STATE;
        command_arg = 0;
    }

    PlayerEnergy(creature_ptr).set_player_turn_energy(100);
    search(creature_ptr);

    if (creature_ptr->action == ACTION_SEARCH)
        search(creature_ptr);
}

static bool exe_alter(player_type *creature_ptr)
{
    DIRECTION dir;
    if (!get_rep_dir(creature_ptr, &dir, true))
        return false;

    POSITION y = creature_ptr->y + ddy[dir];
    POSITION x = creature_ptr->x + ddx[dir];
    grid_type *g_ptr;
    g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
    FEAT_IDX feat = g_ptr->get_feat_mimic();
    feature_type *f_ptr;
    f_ptr = &f_info[feat];
    PlayerEnergy(creature_ptr).set_player_turn_energy(100);
    if (g_ptr->m_idx) {
        do_cmd_attack(creature_ptr, y, x, HISSATSU_NONE);
        return false;
    }
    
    if (f_ptr->flags.has(FF::OPEN))
        return exe_open(creature_ptr, y, x);
    
    if (f_ptr->flags.has(FF::BASH))
        return exe_bash(creature_ptr, y, x, dir);
    
    if (f_ptr->flags.has(FF::TUNNEL))
        return exe_tunnel(creature_ptr, y, x);
    
    if (f_ptr->flags.has(FF::CLOSE))
        return exe_close(creature_ptr, y, x);
    
    if (f_ptr->flags.has(FF::DISARM))
        return exe_disarm(creature_ptr, y, x, dir);

    msg_print(_("何もない空中を攻撃した。", "You attack the empty air."));
    return false;
}

/*!
 * @brief 特定のマスに影響を及ぼすための汎用的コマンド / Manipulate an adjacent grid in some way
 * @details
 */
void do_cmd_alter(player_type *creature_ptr)
{
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (command_arg) {
        command_rep = command_arg - 1;
        creature_ptr->redraw |= PR_STATE;
        command_arg = 0;
    }

    if (!exe_alter(creature_ptr))
        disturb(creature_ptr, false, false);
}

/*!
 * @brief 自殺/引退/切腹の確認
 * @param なし
 * @return 自殺/引退/切腹を実施するならTRUE、キャンセルならFALSE
 */
static bool decide_suicide(void)
{
    if (current_world_ptr->noscore)
        return true;

    prt(_("確認のため '@' を押して下さい。", "Please verify SUICIDE by typing the '@' sign: "), 0, 0);
    flush();
    int i = inkey();
    prt("", 0, 0);
    return i == '@';
}

static void accept_winner_message(player_type *creature_ptr)
{
    if (!current_world_ptr->total_winner || !last_words)
        return;

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

/*!
 * @brief 自殺するコマンドのメインルーチン
 * commit suicide
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

    creature_ptr->last_message = nullptr;
    creature_ptr->playing = false;
    creature_ptr->is_dead = true;
    creature_ptr->leaving = true;
    if (current_world_ptr->total_winner) {
        accept_winner_message(creature_ptr);
        add_retired_class(creature_ptr->pclass);
    } else {
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_GAMEOVER);
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 0, _("ダンジョンの探索に絶望して自殺した。", "gave up all hope to commit suicide."));
        exe_write_diary(creature_ptr, DIARY_GAMESTART, 1, _("-------- ゲームオーバー --------", "--------   Game  Over   --------"));
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, "\n\n\n\n");
    }

    (void)strcpy(creature_ptr->died_from, _("途中終了", "Quitting"));
}
