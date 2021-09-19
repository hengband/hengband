/*!
 * @brief 異常発生時のゲーム緊急終了処理
 * @date 2020/03/01
 * @author Hourier
 * @details
 * Windowsのコードからは呼ばれない。よってVSからは見えない
 */

#include "io/exit-panic.h"
#include "core/disturbance.h"
#include "world/world.h"
#include "player/player-move.h"
#include "io/signal-handlers.h"
#include "save/save.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief Handle abrupt death of the visual system
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return なし
 * @details
 * This routine is called only in very rare situations, and only
 * by certain visual systems, when they experience fatal errors.
 */
void exit_game_panic(player_type *player_ptr)
{
	if (!w_ptr->character_generated || w_ptr->character_saved)
		quit(_("緊急事態", "panic"));
	msg_flag = false;

	prt("", 0, 0);
	disturb(player_ptr, true, true);
	if (player_ptr->chp < 0) player_ptr->is_dead = false;

	player_ptr->panic_save = 1;
	signals_ignore_tstp();
	(void)strcpy(player_ptr->died_from, _("(緊急セーブ)", "(panic save)"));
	if (!save_player(player_ptr, SAVE_TYPE_CLOSE_GAME)) quit(_("緊急セーブ失敗！", "panic save failed!"));
	quit(_("緊急セーブ成功！", "panic save succeeded!"));
}
