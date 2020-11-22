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
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief Handle abrupt death of the visual system
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * This routine is called only in very rare situations, and only
 * by certain visual systems, when they experience fatal errors.
 */
void exit_game_panic(player_type *creature_ptr)
{
	if (!current_world_ptr->character_generated || current_world_ptr->character_saved)
		quit(_("緊急事態", "panic"));
	msg_flag = FALSE;

	prt("", 0, 0);
	disturb(creature_ptr, TRUE, TRUE);
	if (creature_ptr->chp < 0) creature_ptr->is_dead = FALSE;

	creature_ptr->panic_save = 1;
	signals_ignore_tstp();
	(void)strcpy(creature_ptr->died_from, _("(緊急セーブ)", "(panic save)"));
	if (!save_player(creature_ptr)) quit(_("緊急セーブ失敗！", "panic save failed!"));
	quit(_("緊急セーブ成功！", "panic save succeeded!"));
}
