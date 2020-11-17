#include "spell-kind/spells-curse-removal.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-checker.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 装備の解呪処理 / Removes curses from items in inventory
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param all 軽い呪いまでの解除ならば0
 * @return 解呪されたアイテムの数
 * @details 永遠の呪いは解呪できない
 */
static int exe_curse_removal(player_type *creature_ptr, int all)
{
    int cnt = 0;
    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        if (!object_is_cursed(o_ptr))
            continue;
        if (!all && (o_ptr->curse_flags & TRC_HEAVY_CURSE))
            continue;
        if (o_ptr->curse_flags & TRC_PERMA_CURSE) {
            o_ptr->curse_flags &= (TRC_CURSED | TRC_HEAVY_CURSE | TRC_PERMA_CURSE);
            continue;
        }

        o_ptr->curse_flags = 0L;
        o_ptr->ident |= IDENT_SENSE;
        o_ptr->feeling = FEEL_NONE;

        creature_ptr->update |= (PU_BONUS);
        creature_ptr->window |= (PW_EQUIP);
        cnt++;
    }

    if (cnt)
        msg_print(_("誰かに見守られているような気がする。", "You feel as if someone is watching over you."));

    return cnt;
}

/*!
 * @brief 装備の軽い呪い解呪処理 /
 * Remove most curses
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 解呪に成功した装備数
 */
int remove_curse(player_type *caster_ptr) { return exe_curse_removal(caster_ptr, FALSE); }

/*!
 * @brief 装備の重い呪い解呪処理 /
 * Remove all curses
 * @return 解呪に成功した装備数
 */
int remove_all_curse(player_type *caster_ptr) { return exe_curse_removal(caster_ptr, TRUE); }
