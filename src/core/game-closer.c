#include "angband.h"
#include "core/game-closer.h"
#include "view/display-main-window.h"
#include "view/display-player.h"
#include "world.h"
#include "scores.h"
#include "core/stuff-handler.h"
#include "io/signal-handlers.h"
#include "io/uid-checker.h"
#include "main/music-definitions-table.h"
#include "save.h"
#include "player/process-death.h"
#include "cmd/cmd-save.h"

static void clear_floor(player_type* player_ptr)
{
    (void)fd_close(highscore_fd);
    highscore_fd = -1;
    clear_saved_floor_files(player_ptr);
    signals_handle_tstp();
}

/*!
 * @brief ゲーム終了処理 /
 * Close up the current game (player may or may not be dead)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * This function is called only from "main.c" and "signals.c".
 * </pre>
 */
void close_game(player_type* player_ptr)
{
    char buf[1024];
    bool do_send = TRUE;
    handle_stuff(player_ptr);

    msg_print(NULL);
    flush();

    signals_ignore_tstp();

    current_world_ptr->character_icky = TRUE;
    path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");
    safe_setuid_grab();
    highscore_fd = fd_open(buf, O_RDWR);
    safe_setuid_drop();

    if (!player_ptr->is_dead) {
        do_cmd_save_game(player_ptr, FALSE);
        prt(_("リターンキーか ESC キーを押して下さい。", "Press Return (or Escape)."), 0, 40);
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_EXIT);
        if (inkey() != ESCAPE)
            predict_score(player_ptr);

        clear_floor(player_ptr);
        return;
    }

    if (current_world_ptr->total_winner)
        kingly(player_ptr);

    if (!cheat_save || get_check(_("死んだデータをセーブしますか？ ", "Save death? "))) {
        if (!save_player(player_ptr))
            msg_print(_("セーブ失敗！", "death save failed!"));
    } else
        do_send = FALSE;

    print_tomb(player_ptr);
    flush();

    show_info(player_ptr, handle_stuff, update_playtime, display_player, map_name);
    Term_clear();

    if (check_score(player_ptr)) {
        if ((!send_world_score(player_ptr, do_send, update_playtime, display_player, map_name))) {
            if (get_check_strict(_("後でスコアを登録するために待機しますか？", "Stand by for later score registration? "),
                    (CHECK_NO_ESCAPE | CHECK_NO_HISTORY))) {
                player_ptr->wait_report_score = TRUE;
                player_ptr->is_dead = FALSE;
                if (!save_player(player_ptr))
                    msg_print(_("セーブ失敗！", "death save failed!"));
            }
        }

        if (!player_ptr->wait_report_score)
            (void)top_twenty(player_ptr);
    } else if (highscore_fd >= 0) {
        display_scores_aux(0, 10, -1, NULL);
    }

    clear_floor(player_ptr);
}
