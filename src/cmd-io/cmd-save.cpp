#include "cmd-io/cmd-save.h"
#include "cmd-io/cmd-dump.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "io/signal-handlers.h"
#include "io/write-diary.h"
#include "monster/monster-status.h" // 違和感。要調査.
#include "save/save.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief セーブするコマンドのメインルーチン
 * Save the game
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param is_autosave オートセーブ中の処理ならばTRUE
 * @details
 */
void do_cmd_save_game(PlayerType *player_ptr, int is_autosave)
{
    if (is_autosave) {
        msg_print(_("自動セーブ中", "Autosaving the game..."));
    } else {
        disturb(player_ptr, true, true);
    }

    msg_print(nullptr);
    handle_stuff(player_ptr);
    prt(_("ゲームをセーブしています...", "Saving game..."), 0, 0);
    term_fresh();
    player_ptr->died_from = _("(セーブ)", "(saved)");
    signals_ignore_tstp();
    if (save_player(player_ptr, SaveType::CONTINUE_GAME)) {
        prt(_("ゲームをセーブしています... 終了", "Saving game... done."), 0, 0);
    } else {
        prt(_("ゲームをセーブしています... 失敗！", "Saving game... failed!"), 0, 0);
    }

    signals_handle_tstp();
    term_fresh();
    player_ptr->died_from = _("(元気に生きている)", "(alive and well)");
}

/*!
 * @brief セーブ後にゲーム中断フラグを立てる/
 * Save the game and exit
 * @details
 */
void do_cmd_save_and_exit(PlayerType *player_ptr)
{
    player_ptr->playing = false;
    player_ptr->leaving = true;
    exe_write_diary(player_ptr, DiaryKind::GAMESTART, 0, _("----ゲーム中断----", "--- Saved and Exited Game ---"));
}
