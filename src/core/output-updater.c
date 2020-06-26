/*!
 * @brief メインウィンドウの再描画を行う
 * @date 2020/05/09
 * @author Hourier
 */

#include "core/output-updater.h"
#include "core/window-redrawer.h"

void update_output(player_type* player_ptr)
{
    if (player_ptr->redraw)
        redraw_stuff(player_ptr);
    if (player_ptr->window)
        window_stuff(player_ptr);
}
