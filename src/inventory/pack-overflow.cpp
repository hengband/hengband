#include "inventory/pack-overflow.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "object/object-info.h"
#include "player/player-status.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief アイテムの所持種類数が超えた場合にアイテムを床に落とす処理
 */
void pack_overflow(PlayerType *player_ptr)
{
    if (player_ptr->inventory_list[INVEN_PACK].k_idx == 0) {
        return;
    }

    GAME_TEXT o_name[MAX_NLEN];
    ItemEntity *o_ptr;
    update_creature(player_ptr);
    if (!player_ptr->inventory_list[INVEN_PACK].k_idx) {
        return;
    }

    o_ptr = &player_ptr->inventory_list[INVEN_PACK];
    disturb(player_ptr, false, true);
    msg_print(_("ザックからアイテムがあふれた！", "Your pack overflows!"));

    describe_flavor(player_ptr, o_name, o_ptr, 0);
    msg_format(_("%s(%c)を落とした。", "You drop %s (%c)."), o_name, index_to_label(INVEN_PACK));
    (void)drop_near(player_ptr, o_ptr, 0, player_ptr->y, player_ptr->x);

    vary_item(player_ptr, INVEN_PACK, -255);
    handle_stuff(player_ptr);
}
