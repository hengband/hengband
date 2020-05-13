#include "angband.h"
#include "core/stuff-handler.h"
#include "view/display-main-window.h"

/*!
 * @brief 全更新処理をチェックして処理していく
 * Handle "player_ptr->update" and "player_ptr->redraw" and "player_ptr->window"
 * @return なし
 */
void handle_stuff(player_type* player_ptr)
{
    if (player_ptr->update)
        update_creature(player_ptr);
    if (player_ptr->redraw)
        redraw_stuff(player_ptr);
    if (player_ptr->window)
        window_stuff(player_ptr);
}
