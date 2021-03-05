﻿#include "load/inventory-loader.h"
#include "inventory/inventory-slot-types.h"
#include "load/item-loader.h"
#include "load/load-util.h"
#include "object/object-generator.h"
#include "object/object-mark-types.h"
#include "system/object-type-definition.h"

/*!
 * @brief プレイヤーの所持品情報を読み込む / Read the player inventory
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * Note that the inventory changed in Angband 2.7.4.  Two extra
 * pack slots were added and the equipment was rearranged.  Note
 * that these two features combine when parsing old save-files, in
 * which items from the old "aux" slot are "carried", perhaps into
 * one of the two new "inventory" slots.
 *
 * Note that the inventory is "re-sorted" later by "dungeon()".
 */
static errr rd_inventory(player_type *player_ptr)
{
    player_ptr->inven_cnt = 0;
    player_ptr->equip_cnt = 0;

    if (player_ptr->inventory_list != NULL)
        C_WIPE(player_ptr->inventory_list, INVEN_TOTAL, object_type);
    C_MAKE(player_ptr->inventory_list, INVEN_TOTAL, object_type);

    int slot = 0;
    while (TRUE) {
        u16b n;
        rd_u16b(&n);

        if (n == 0xFFFF)
            break;
        object_type forge;
        object_type *q_ptr;
        q_ptr = &forge;
        object_wipe(q_ptr);

        rd_item(player_ptr, q_ptr);
        if (!q_ptr->k_idx)
            return (53);

        if (n >= INVEN_MAIN_HAND) {
            q_ptr->marked |= OM_TOUCHED;
            object_copy(&player_ptr->inventory_list[n], q_ptr);
            player_ptr->equip_cnt++;
            continue;
        }

        if (player_ptr->inven_cnt == INVEN_PACK) {
            load_note(_("持ち物の中のアイテムが多すぎる！", "Too many items in the inventory"));
            return (54);
        }

        n = slot++;
        q_ptr->marked |= OM_TOUCHED;
        object_copy(&player_ptr->inventory_list[n], q_ptr);
        player_ptr->inven_cnt++;
    }

    return 0;
}

errr load_inventory(player_type *creature_ptr)
{
    byte tmp8u;
    for (int i = 0; i < 64; i++) {
        rd_byte(&tmp8u);
        creature_ptr->spell_order[i] = (SPELL_IDX)tmp8u;
    }

    if (!rd_inventory(creature_ptr))
        return 0;

    load_note(_("持ち物情報を読み込むことができません", "Unable to read inventory"));
    return 21;
}
