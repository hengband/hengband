/*!
 * @file autopick.c
 * @brief 自動拾い機能の実装 / Object Auto-picker/Destroyer
 * @date 2014/01/02
 * @author
 * Copyright (c) 2002  Mogami\n
 *\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "autopick/autopick.h"
#include "autopick/autopick-destroyer.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-menu-data-table.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/player-inventory.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include <sstream>

/*!
 * @brief Auto-destroy marked item
 */
static void autopick_delayed_alter_aux(PlayerType *player_ptr, INVENTORY_IDX item)
{
    const auto *o_ptr = ref_item(player_ptr, item);
    if (!o_ptr->is_valid() || o_ptr->marked.has_not(OmType::AUTODESTROY)) {
        return;
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
    if (item >= 0) {
        inven_item_increase(player_ptr, item, -(o_ptr->number));
        inven_item_optimize(player_ptr, item);
    } else {
        delete_object_idx(player_ptr, 0 - item);
    }

    msg_format(_("%sを自動破壊します。", "Auto-destroying %s."), item_name.data());
}

/*!
 * @brief Auto-destroy marked items in inventry and on floor
 * @details
 * Scan inventry in reverse order to prevent
 * skipping after inven_item_optimize()
 */
void autopick_delayed_alter(PlayerType *player_ptr)
{
    for (INVENTORY_IDX item = INVEN_TOTAL - 1; item >= 0; item--) {
        autopick_delayed_alter_aux(player_ptr, item);
    }

    auto &grid = player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    for (auto it = grid.o_idx_list.begin(); it != grid.o_idx_list.end();) {
        INVENTORY_IDX item = *it++;
        autopick_delayed_alter_aux(player_ptr, -item);
    }

    // PW_FLOOR_ITEM_LISTは遅れるので即時更新
    fix_floor_item_list(player_ptr, player_ptr->y, player_ptr->x);
}

/*!
 * @brief Auto-inscription and/or destroy
 * @details
 * Auto-destroyer works only on inventory or on floor stack only when
 * requested.
 */
void autopick_alter_item(PlayerType *player_ptr, INVENTORY_IDX item, bool destroy)
{
    ItemEntity *o_ptr;
    o_ptr = ref_item(player_ptr, item);
    int idx = find_autopick_list(player_ptr, o_ptr);
    auto_inscribe_item(o_ptr, idx);
    if (destroy && item <= INVEN_PACK) {
        auto_destroy_item(player_ptr, o_ptr, idx);
    }
}

/*!
 * @brief Automatically pickup/destroy items in this grid.
 */
void autopick_pickup_items(PlayerType *player_ptr, grid_type *g_ptr)
{
    for (auto it = g_ptr->o_idx_list.begin(); it != g_ptr->o_idx_list.end();) {
        OBJECT_IDX this_o_idx = *it++;
        auto *o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
        int idx = find_autopick_list(player_ptr, o_ptr);
        auto_inscribe_item(o_ptr, idx);
        if ((idx < 0) || (autopick_list[idx].action & (DO_AUTOPICK | DO_QUERY_AUTOPICK)) == 0) {
            auto_destroy_item(player_ptr, o_ptr, idx);
            continue;
        }

        disturb(player_ptr, false, false);
        if (!check_store_item_to_inventory(player_ptr, o_ptr)) {
            const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
            msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), item_name.data());
            o_ptr->marked.set(OmType::SUPRESS_MESSAGE);
            continue;
        }

        if (!(autopick_list[idx].action & DO_QUERY_AUTOPICK)) {
            describe_pickup_item(player_ptr, this_o_idx);
            continue;
        }

        if (o_ptr->marked.has(OmType::NO_QUERY)) {
            continue;
        }

        const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
        std::stringstream ss;
        ss << _(item_name, "Pick up ") << _("を拾いますか", item_name) << "? ";
        if (!input_check(ss.str())) {
            o_ptr->marked.set({ OmType::SUPRESS_MESSAGE, OmType::NO_QUERY });
            continue;
        }

        describe_pickup_item(player_ptr, this_o_idx);
    }
}
