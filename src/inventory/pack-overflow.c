#include "inventory/pack-overflow.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "object/object-info.h"
#include "view/display-messages.h"

/*!
 * @brief アイテムの所持種類数が超えた場合にアイテムを床に落とす処理
 * @return なし
 */
void pack_overflow(player_type *owner_ptr)
{
    if (owner_ptr->inventory_list[INVEN_PACK].k_idx == 0)
        return;

    GAME_TEXT o_name[MAX_NLEN];
    object_type *o_ptr;
    update_creature(owner_ptr);
    if (!owner_ptr->inventory_list[INVEN_PACK].k_idx)
        return;

    o_ptr = &owner_ptr->inventory_list[INVEN_PACK];
    disturb(owner_ptr, FALSE, TRUE);
    msg_print(_("ザックからアイテムがあふれた！", "Your pack overflows!"));

    describe_flavor(owner_ptr, o_name, o_ptr, 0);
    msg_format(_("%s(%c)を落とした。", "You drop %s (%c)."), o_name, index_to_label(INVEN_PACK));
    (void)drop_near(owner_ptr, o_ptr, 0, owner_ptr->y, owner_ptr->x);

    vary_item(owner_ptr, INVEN_PACK, -255);
    handle_stuff(owner_ptr);
}
