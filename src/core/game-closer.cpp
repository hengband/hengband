/*
 * @file game-closer.cpp
 * @brief ゲーム終了処理
 * @author Hourier
 * @date 2020/03/09
 */

#include "core/game-closer.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "core/score-util.h"
#include "core/scores.h"
#include "core/stuff-handler.h"
#include "floor/floor-save.h"
#include "game-option/cheat-options.h"
#include "io/input-key-acceptor.h"
#include "io/signal-handlers.h"
#include "io/uid-checker.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "player/player-sex.h"
#include "player/process-death.h"
#include "save/save.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "view/display-scores.h"
#include "world/world.h"

static void clear_floor(player_type *player_ptr)
{
    (void)fd_close(highscore_fd);
    highscore_fd = -1;
    clear_saved_floor_files(player_ptr);
    signals_handle_tstp();
}

static void send_world_score_on_closing(player_type *player_ptr, bool do_send)
{
    if (send_world_score(player_ptr, do_send, display_player))
        return;

    if (!get_check_strict(
            player_ptr, _("後でスコアを登録するために待機しますか？", "Stand by for later score registration? "), (CHECK_NO_ESCAPE | CHECK_NO_HISTORY)))
        return;

    player_ptr->wait_report_score = true;
    player_ptr->is_dead = false;
    if (!save_player(player_ptr, SAVE_TYPE_CLOSE_GAME))
        msg_print(_("セーブ失敗！", "death save failed!"));
}

/*!
 * @brief ゲームクローズ時、プレイヤーが死亡しているかのチェックを行い死亡していないならば、確認キー入力とスコア表示、現フロアの初期化を行う。
 * @param player_ptr プレイヤー構造体参照ポインタ。
 * @return 死亡していればTRUE, まだ生きているならば各処理を済ませた上ででFALSE。
 */
static bool check_death(player_type *player_ptr)
{
    if (player_ptr->is_dead)
        return true;

    do_cmd_save_game(player_ptr, false);
    prt(_("リターンキーか ESC キーを押して下さい。", "Press Return (or Escape)."), 0, 40);
    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_EXIT);
    if (inkey() != ESCAPE)
        predict_score(player_ptr);

    clear_floor(player_ptr);
    return false;
}

/*!
 * @brief 勝利者用の引退演出処理 /
 * Change the player into a King! -RAK-
 */
static void kingly(player_type *player_ptr)
{
    bool seppuku = streq(player_ptr->died_from, "Seppuku");
    player_ptr->current_floor_ptr->dun_level = 0;
    if (!seppuku) {
        /* 引退したときの識別文字 */
        (void)strcpy(player_ptr->died_from, _("ripe", "Ripe Old Age"));
    }

    player_ptr->exp = player_ptr->max_exp;
    player_ptr->lev = player_ptr->max_plv;
    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);
    auto cy = hgt / 2;
    auto cx = wid / 2;
    player_ptr->au += 10000000L;
    term_clear();

    put_str("#", cy - 11, cx - 1);
    put_str("#####", cy - 10, cx - 3);
    put_str("#", cy - 9, cx - 1);
    put_str(",,,  $$$  ,,,", cy - 8, cx - 7);
    put_str(",,=$   \"$$$$$\"   $=,,", cy - 7, cx - 11);
    put_str(",$$        $$$        $$,", cy - 6, cx - 13);
    put_str("*>         <*>         <*", cy - 5, cx - 13);
    put_str("$$         $$$         $$", cy - 4, cx - 13);
    put_str("\"$$        $$$        $$\"", cy - 3, cx - 13);
    put_str("\"$$       $$$       $$\"", cy - 2, cx - 12);
    put_str("*#########*#########*", cy - 1, cx - 11);
    put_str("*#########*#########*", cy, cx - 11);

#ifdef JP
    put_str("Veni, Vidi, Vici!", cy + 3, cx - 9);
    put_str("来た、見た、勝った！", cy + 4, cx - 10);
    put_str(format("偉大なる%s万歳！", sp_ptr->winner), cy + 5, cx - 11);
#else
    put_str("Veni, Vidi, Vici!", cy + 3, cx - 9);
    put_str("I came, I saw, I conquered!", cy + 4, cx - 14);
    put_str(format("All Hail the Mighty %s!", sp_ptr->winner), cy + 5, cx - 13);
#endif

    if (!seppuku) {
        exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, _("ダンジョンの探索から引退した。", "retired exploring dungeons."));
        exe_write_diary(player_ptr, DIARY_GAMESTART, 1, _("-------- ゲームオーバー --------", "--------   Game  Over   --------"));
        exe_write_diary(player_ptr, DIARY_DESCRIPTION, 1, "\n\n\n\n");
    }

    flush();
    pause_line(hgt - 1);
}

/*!
 * @brief ゲーム終了処理 /
 * Close up the current game (player may or may not be dead)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * <pre>
 * This function is called only from "main.c" and "signals.c".
 * </pre>
 */
void close_game(player_type *player_ptr)
{
    bool do_send = true;
    handle_stuff(player_ptr);
    msg_print(nullptr);
    flush();
    signals_ignore_tstp();

    current_world_ptr->character_icky_depth = 1;
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");
    safe_setuid_grab(player_ptr);
    highscore_fd = fd_open(buf, O_RDWR);
    safe_setuid_drop();

    if (!check_death(player_ptr))
        return;

    if (current_world_ptr->total_winner)
        kingly(player_ptr);

    if (!cheat_save || get_check(_("死んだデータをセーブしますか？ ", "Save death? "))) {
        update_playtime();
        current_world_ptr->sf_play_time += current_world_ptr->play_time;

        if (!save_player(player_ptr, SAVE_TYPE_CLOSE_GAME))
            msg_print(_("セーブ失敗！", "death save failed!"));
    } else
        do_send = false;

    print_tomb(player_ptr);
    flush();
    show_death_info(player_ptr, display_player);
    term_clear();
    if (check_score(player_ptr)) {
        send_world_score_on_closing(player_ptr, do_send);
        if (!player_ptr->wait_report_score)
            (void)top_twenty(player_ptr);
    } else if (highscore_fd >= 0) {
        display_scores(0, 10, -1, nullptr);
    }

    clear_floor(player_ptr);
}
