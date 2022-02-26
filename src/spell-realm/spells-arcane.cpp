#include "spell-realm/spells-arcane.h"
#include "core/player-update-types.h"
#include "inventory/inventory-slot-types.h"
#include "object/tval-types.h"
#include "sv-definition/sv-lite-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 寿命つき光源の燃素追加処理 /
 * Charge a lite (torch or latern)
 */
void phlogiston(PlayerType *player_ptr)
{
    short max_flog = 0;
    auto *o_ptr = &player_ptr->inventory_list[INVEN_LITE];
    if ((o_ptr->tval == ItemKindType::LITE) && (o_ptr->sval == SV_LITE_LANTERN)) {
        max_flog = FUEL_LAMP;
    } else if ((o_ptr->tval == ItemKindType::LITE) && (o_ptr->sval == SV_LITE_TORCH)) {
        max_flog = FUEL_TORCH;
    } else {
        msg_print(_("燃素を消費するアイテムを装備していません。", "You are not wielding anything which uses phlogiston."));
        return;
    }

    if (o_ptr->fuel >= max_flog) {
        msg_print(_("このアイテムにはこれ以上燃素を補充できません。", "No more phlogiston can be put in this item."));
        return;
    }

    o_ptr->fuel += max_flog / 2;
    msg_print(_("照明用アイテムに燃素を補充した。", "You add phlogiston to your light."));
    if (o_ptr->fuel >= max_flog) {
        o_ptr->fuel = max_flog;
        msg_print(_("照明用アイテムは満タンになった。", "Your light is full."));
    }

    player_ptr->update |= PU_TORCH;
}
