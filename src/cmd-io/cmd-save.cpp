#include "cmd-io/cmd-save.h"
#include "cmd-io/cmd-dump.h"
#include "io/signal-handlers.h"
#include "io/write-diary.h"
#include "monster/monster-status.h" // 違和感。要調査.
#include "save/save.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 自動セーブを要求されていて、まだセーブしていないか
 */
static bool is_auto_save_requested = false;

/*!
 * @brief セーブするコマンドのメインルーチン
 * Save the game
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param is_autosave オートセーブ中の処理ならばTRUE
 * @details
 */
void do_cmd_save_game(PlayerType *player_ptr, int is_autosave)
{
    // どのセーブでも、要求されていた自動セーブの分を兼ねる
    is_auto_save_requested = false;
    if (is_autosave) {
        msg_print(_("自動セーブ中", "Autosaving the game..."));
    }

    msg_erase();
    prt(_("ゲームをセーブしています...", "Saving game..."), 0, 0);
    term_fresh();
    player_ptr->died_from = _("(セーブ)", "(saved)");
    signals_ignore_tstp();
    if (save_player(player_ptr)) {
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
    exe_write_diary(*player_ptr->current_floor_ptr, DiaryKind::GAMESTART, 0, _("----ゲーム中断----", "--- Saved and Exited Game ---"));
}

/*!
 * @brief 自動セーブ (一定ターン毎・新しい階に入るとき) を要求する
 * @details 次にコマンドの入力を待つところで do_requested_auto_save() がセーブする。
 * 階を移る処理 (階段・テレポート・レベル・帰還の発動など) の途中で要求したときは、移った先の階でセーブする
 */
void request_auto_save()
{
    is_auto_save_requested = true;
}

/*!
 * @brief 要求されている自動セーブを行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details コマンドの入力を待つところで呼ぶ (process_player())。読み込むと、^S でセーブしたときと同じく手番の途中から再開する。
 */
void do_requested_auto_save(PlayerType *player_ptr)
{
    if (!is_auto_save_requested) {
        return;
    }

    do_cmd_save_game(player_ptr, true);
}
