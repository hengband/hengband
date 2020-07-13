#include "inventory/player-inventory.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "floor/floor.h"
#include "floor/object-scanner.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-mark-types.h"
#include "player/player-move.h"
#include "view/display-messages.h"

/*!
 * @brief 規定の処理にできるアイテムがプレイヤーの利用可能範囲内にあるかどうかを返す /
 * Determine whether get_item() can get some item or not
 * @return アイテムを拾えるならばTRUEを返す。
 * @details assuming mode = (USE_EQUIP | USE_INVEN | USE_FLOOR).
 */
bool can_get_item(player_type *owner_ptr, tval_type tval)
{
    for (int j = 0; j < INVEN_TOTAL; j++)
        if (item_tester_okay(owner_ptr, &owner_ptr->inventory_list[j], tval))
            return TRUE;

    OBJECT_IDX floor_list[23];
    ITEM_NUMBER floor_num = scan_floor_items(owner_ptr, floor_list, owner_ptr->y, owner_ptr->x, 0x03, tval);
    return floor_num != 0;
}

/*!
 * @brief 床上のアイテムを拾う選択用サブルーチン
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す。
 */
static bool py_pickup_floor_aux(player_type *owner_ptr)
{
    OBJECT_IDX this_o_idx;
    OBJECT_IDX item;
    item_tester_hook = check_store_item_to_inventory;
    concptr q = _("どれを拾いますか？", "Get which item? ");
    concptr s = _("もうザックには床にあるどのアイテムも入らない。", "You no longer have any room for the objects on the floor.");
    if (choose_object(owner_ptr, &item, q, s, (USE_FLOOR), 0))
        this_o_idx = 0 - item;
    else
        return FALSE;

    py_pickup_aux(owner_ptr, this_o_idx);
    return TRUE;
}

/*!
 * @brief 床上のアイテムを拾うメイン処理
 * @param pickup FALSEなら金銭の自動拾いのみを行う/ FALSE then only gold will be picked up
 * @return なし
 * @details
 * This is called by py_pickup() when easy_floor is TRUE.
 */
void py_pickup_floor(player_type *owner_ptr, bool pickup)
{
    OBJECT_IDX this_o_idx, next_o_idx = 0;
    GAME_TEXT o_name[MAX_NLEN];
    object_type *o_ptr;
    int floor_num = 0;
    OBJECT_IDX floor_o_idx = 0;
    int can_pickup = 0;
    for (this_o_idx = owner_ptr->current_floor_ptr->grid_array[owner_ptr->y][owner_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx) {
        o_ptr = &owner_ptr->current_floor_ptr->o_list[this_o_idx];
        describe_flavor(owner_ptr, o_name, o_ptr, 0);
        next_o_idx = o_ptr->next_o_idx;
        disturb(owner_ptr, FALSE, FALSE);
        if (o_ptr->tval == TV_GOLD) {
            msg_format(_(" $%ld の価値がある%sを見つけた。", "You have found %ld gold pieces worth of %s."), (long)o_ptr->pval, o_name);
            owner_ptr->au += o_ptr->pval;
            owner_ptr->redraw |= (PR_GOLD);
            owner_ptr->window |= (PW_PLAYER);
            delete_object_idx(owner_ptr, this_o_idx);
            continue;
        } else if (o_ptr->marked & OM_NOMSG) {
            o_ptr->marked &= ~(OM_NOMSG);
            continue;
        }

        if (check_store_item_to_inventory(owner_ptr, o_ptr))
            can_pickup++;

        floor_num++;
        floor_o_idx = this_o_idx;
    }

    if (!floor_num)
        return;

    if (!pickup) {
        if (floor_num == 1) {
            o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
            describe_flavor(owner_ptr, o_name, o_ptr, 0);
            msg_format(_("%sがある。", "You see %s."), o_name);
        } else
            msg_format(_("%d 個のアイテムの山がある。", "You see a pile of %d items."), floor_num);

        return;
    }

    if (!can_pickup) {
        if (floor_num == 1) {
            o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
            describe_flavor(owner_ptr, o_name, o_ptr, 0);
            msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), o_name);
        } else
            msg_print(_("ザックには床にあるどのアイテムも入らない。", "You have no room for any of the objects on the floor."));

        return;
    }

    if (floor_num != 1) {
        while (can_pickup--)
            if (!py_pickup_floor_aux(owner_ptr))
                break;

        return;
    }

    if (carry_query_flag) {
        char out_val[MAX_NLEN + 20];
        o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
        describe_flavor(owner_ptr, o_name, o_ptr, 0);
        (void)sprintf(out_val, _("%sを拾いますか? ", "Pick up %s? "), o_name);
        if (!get_check(out_val))
            return;
    }

    o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
    py_pickup_aux(owner_ptr, floor_o_idx);
}
