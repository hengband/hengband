#include "cmd-io/cmd-help.h"
#include "core/show-file.h"
#include "term/screen-processor.h"

/*!
 * @brief ヘルプを表示するコマンドのメインルーチン
 * Peruse the On-Line-Help
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 */
void do_cmd_help(player_type *creature_ptr)
{
	screen_save();
	(void)show_file(creature_ptr, TRUE, _("jhelp.hlp", "help.hlp"), NULL, 0, 0);
	screen_load();
}
