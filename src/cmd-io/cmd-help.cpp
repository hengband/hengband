#include "cmd-io/cmd-help.h"
#include "core/show-file.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"

/*!
 * @brief ヘルプを表示するコマンドのメインルーチン
 * Peruse the On-Line-Help
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @details
 */
void do_cmd_help(player_type *creature_ptr)
{
	screen_save();
	(void)show_file(creature_ptr, true, _("jhelp.hlp", "help.hlp"), nullptr, 0, 0);
	screen_load();
}
