#include "cmd-io/cmd-help.h"
#include "core/show-file.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"

/*!
 * @brief ヘルプを表示するコマンドのメインルーチン
 * Peruse the On-Line-Help
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 */
void do_cmd_help(PlayerType *player_ptr)
{
    screen_save();
    FileDisplayer(player_ptr->name).display(true, _("jhelp.hlp", "help.hlp"), 0, 0);
    screen_load();
}
