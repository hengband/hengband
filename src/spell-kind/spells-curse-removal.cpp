#include "spell-kind/spells-curse-removal.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 装備の解呪処理 / Removes curses from items in inventory
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param all 軽い呪いまでの解除ならば0
 * @return 解呪されたアイテムの数
 * @details 永遠の呪いは解呪できない
 */
static int exe_curse_removal(player_type *player_ptr, int all)
{
    int cnt = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        if (!o_ptr->is_cursed())
            continue;
        if (!all && o_ptr->curse_flags.has(TRC::HEAVY_CURSE))
            continue;
        if (o_ptr->curse_flags.has(TRC::PERMA_CURSE)) {
            o_ptr->curse_flags &= {TRC::CURSED, TRC::HEAVY_CURSE, TRC::PERMA_CURSE};
            continue;
        }

        o_ptr->curse_flags.clear();
        o_ptr->ident |= IDENT_SENSE;
        o_ptr->feeling = FEEL_NONE;

        player_ptr->update |= (PU_BONUS);
        player_ptr->window_flags |= (PW_EQUIP);
        cnt++;
    }

    if (cnt)
        msg_print(_("誰かに見守られているような気がする。", "You feel as if someone is watching over you."));

    return cnt;
}

/*!
 * @brief 装備の軽い呪い解呪処理 /
 * Remove most curses
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 解呪に成功した装備数
 */
int remove_curse(player_type *player_ptr) { return exe_curse_removal(player_ptr, false); }

/*!
 * @brief 装備の重い呪い解呪処理 /
 * Remove all curses
 * @return 解呪に成功した装備数
 */
int remove_all_curse(player_type *player_ptr) { return exe_curse_removal(player_ptr, true); }
