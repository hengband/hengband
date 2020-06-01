#include "inventory/inventory-object.h"
#include "object/object2.h" // 暫定、相互参照している.
#include "view/object-describer.h"

void vary_item(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER num)
{
    if (item >= 0) {
        inven_item_increase(owner_ptr, item, num);
        inven_item_describe(owner_ptr, item);
        inven_item_optimize(owner_ptr, item);
        return;
    }

    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    floor_item_increase(floor_ptr, 0 - item, num);
    floor_item_describe(owner_ptr, 0 - item);
    floor_item_optimize(owner_ptr, 0 - item);
}

/*!
 * @brief アイテムを増減させ残り所持数メッセージを表示する /
 * Increase the "number" of an item in the inventory
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param item 所持数を増やしたいプレイヤーのアイテム所持スロット
 * @param num 増やしたい量
 * @return なし
 */
void inven_item_increase(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER num)
{
    object_type *o_ptr = &owner_ptr->inventory_list[item];
    num += o_ptr->number;
    if (num > 255)
        num = 255;
    else if (num < 0)
        num = 0;

    num -= o_ptr->number;
    if (num == 0)
        return;

    o_ptr->number += num;
    owner_ptr->total_weight += (num * o_ptr->weight);
    owner_ptr->update |= (PU_BONUS);
    owner_ptr->update |= (PU_MANA);
    owner_ptr->update |= (PU_COMBINE);
    owner_ptr->window |= (PW_INVEN | PW_EQUIP);

    if (o_ptr->number || !owner_ptr->ele_attack)
        return;
    if (!(item == INVEN_RARM) && !(item == INVEN_LARM))
        return;
    if (has_melee_weapon(owner_ptr, INVEN_RARM + INVEN_LARM - item))
        return;

    set_ele_attack(owner_ptr, 0, 0);
}

/*!
 * @brief 所持アイテムスロットから所持数のなくなったアイテムを消去する /
 * Erase an inventory slot if it has no more items
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param item 消去したいプレイヤーのアイテム所持スロット
 * @return なし
 */
void inven_item_optimize(player_type *owner_ptr, INVENTORY_IDX item)
{
    object_type *o_ptr = &owner_ptr->inventory_list[item];
    if (!o_ptr->k_idx)
        return;
    if (o_ptr->number)
        return;

    if (item >= INVEN_RARM) {
        owner_ptr->equip_cnt--;
        object_wipe(&owner_ptr->inventory_list[item]);
        owner_ptr->update |= PU_BONUS;
        owner_ptr->update |= PU_TORCH;
        owner_ptr->update |= PU_MANA;

        owner_ptr->window |= PW_EQUIP;
        owner_ptr->window |= PW_SPELL;
        return;
    }

    owner_ptr->inven_cnt--;
    int i;
    for (i = item; i < INVEN_PACK; i++) {
        owner_ptr->inventory_list[i] = owner_ptr->inventory_list[i + 1];
    }

    object_wipe(&owner_ptr->inventory_list[i]);
    owner_ptr->window |= PW_INVEN;
    owner_ptr->window |= PW_SPELL;
}
