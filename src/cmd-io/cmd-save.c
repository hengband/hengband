#include "cmd-io/cmd-save.h"
#include "cmd-io/cmd-dump.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "io/signal-handlers.h"
#include "io/write-diary.h"
#include "monster/monster-status.h" // 違和感。要調査.
#include "save/save.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief セーブするコマンドのメインルーチン
 * Save the game
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param is_autosave オートセーブ中の処理ならばTRUE
 * @return なし
 * @details
 */
void do_cmd_save_game(player_type *creature_ptr, int is_autosave)
{
	if (is_autosave)
		msg_print(_("自動セーブ中", "Autosaving the game..."));
	else
		disturb(creature_ptr, TRUE, TRUE);

	msg_print(NULL);
	handle_stuff(creature_ptr);
	prt(_("ゲームをセーブしています...", "Saving game..."), 0, 0);
	term_fresh();
	(void)strcpy(creature_ptr->died_from, _("(セーブ)", "(saved)"));
	signals_ignore_tstp();
	if (save_player(creature_ptr))
		prt(_("ゲームをセーブしています... 終了", "Saving game... done."), 0, 0);
	else
		prt(_("ゲームをセーブしています... 失敗！", "Saving game... failed!"), 0, 0);

	signals_handle_tstp();
	term_fresh();
	(void)strcpy(creature_ptr->died_from, _("(元気に生きている)", "(alive and well)"));
	current_world_ptr->is_loading_now = FALSE;
	update_creature(creature_ptr);
	mproc_init(creature_ptr->current_floor_ptr);
	current_world_ptr->is_loading_now = TRUE;
}


/*!
 * @brief セーブ後にゲーム中断フラグを立てる/
 * Save the game and exit
 * @return なし
 * @details
 */
void do_cmd_save_and_exit(player_type *creature_ptr)
{
	creature_ptr->playing = FALSE;
	creature_ptr->leaving = TRUE;
	exe_write_diary(creature_ptr, DIARY_GAMESTART, 0, _("----ゲーム中断----", "--- Saved and Exited Game ---"));
}
