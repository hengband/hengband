#include "inventory/inventory-object.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "inventory/inventory-slot-types.h"
#include "object-hook/hook-weapon.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "player-info/equipment-info.h"
#include "spell-realm/spells-craft.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/object-sort.h"
#include "view/display-messages.h"
#include "view/object-describer.h"

void vary_item(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER num)
{
    if (item >= 0) {
        inven_item_increase(owner_ptr, item, num);
        inven_item_describe(owner_ptr, item);
        inven_item_optimize(owner_ptr, item);
        return;
    }

    floor_item_increase(owner_ptr, 0 - item, num);
    floor_item_describe(owner_ptr, 0 - item);
    floor_item_optimize(owner_ptr, 0 - item);
}

/*!
 * @brief アイテムを増減させ残り所持数メッセージを表示する /
 * Increase the "number" of an item in the inventory
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param item 所持数を増やしたいプレイヤーのアイテム所持スロット
 * @param num 増やしたい量
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
    owner_ptr->update |= (PU_BONUS);
    owner_ptr->update |= (PU_MANA);
    owner_ptr->update |= (PU_COMBINE);
    owner_ptr->window_flags |= (PW_INVEN | PW_EQUIP);

    if (o_ptr->number || !owner_ptr->ele_attack)
        return;
    if (!(item == INVEN_MAIN_HAND) && !(item == INVEN_SUB_HAND))
        return;
    if (has_melee_weapon(owner_ptr, INVEN_MAIN_HAND + INVEN_SUB_HAND - item))
        return;

    set_ele_attack(owner_ptr, 0, 0);
}

/*!
 * @brief 所持アイテムスロットから所持数のなくなったアイテムを消去する /
 * Erase an inventory slot if it has no more items
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param item 消去したいプレイヤーのアイテム所持スロット
 */
void inven_item_optimize(player_type *owner_ptr, INVENTORY_IDX item)
{
    object_type *o_ptr = &owner_ptr->inventory_list[item];
    if (!o_ptr->k_idx)
        return;
    if (o_ptr->number)
        return;

    if (item >= INVEN_MAIN_HAND) {
        owner_ptr->equip_cnt--;
        (&owner_ptr->inventory_list[item])->wipe();
        owner_ptr->update |= PU_BONUS;
        owner_ptr->update |= PU_TORCH;
        owner_ptr->update |= PU_MANA;

        owner_ptr->window_flags |= PW_EQUIP;
        owner_ptr->window_flags |= PW_SPELL;
        return;
    }

    owner_ptr->inven_cnt--;
    int i;
    for (i = item; i < INVEN_PACK; i++) {
        owner_ptr->inventory_list[i] = owner_ptr->inventory_list[i + 1];
    }

    (&owner_ptr->inventory_list[i])->wipe();
    owner_ptr->window_flags |= PW_INVEN;
    owner_ptr->window_flags |= PW_SPELL;
}

/*!
 * @brief 所持スロットから床下にオブジェクトを落とすメインルーチン /
 * Drop (some of) a non-cursed inventory/equipment item
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param item 所持テーブルのID
 * @param amt 落としたい個数
 * @details
 * The object will be dropped "near" the current location
 */
void drop_from_inventory(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER amt)
{
    object_type forge;
    object_type *q_ptr;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    o_ptr = &owner_ptr->inventory_list[item];
    if (amt <= 0)
        return;

    if (amt > o_ptr->number)
        amt = o_ptr->number;

    if (item >= INVEN_MAIN_HAND) {
        item = inven_takeoff(owner_ptr, item, amt);
        o_ptr = &owner_ptr->inventory_list[item];
    }

    q_ptr = &forge;
    q_ptr->copy_from(o_ptr);
    distribute_charges(o_ptr, q_ptr, amt);

    q_ptr->number = amt;
    describe_flavor(owner_ptr, o_name, q_ptr, 0);
    msg_format(_("%s(%c)を落とした。", "You drop %s (%c)."), o_name, index_to_label(item));
    (void)drop_near(owner_ptr, q_ptr, 0, owner_ptr->y, owner_ptr->x);
    vary_item(owner_ptr, item, -amt);
}

/*!
 * @brief プレイヤーの所持スロットに存在するオブジェクトをまとめなおす /
 * Combine items in the pack
 * @details
 * Note special handling of the "overflow" slot
 */
void combine_pack(player_type *owner_ptr)
{
    bool flag = false;
    bool is_first_combination = true;
    bool combined = true;
    while (is_first_combination || combined) {
        is_first_combination = false;
        combined = false;

        for (int i = INVEN_PACK; i > 0; i--) {
            object_type *o_ptr;
            o_ptr = &owner_ptr->inventory_list[i];
            if (!o_ptr->k_idx)
                continue;
            for (int j = 0; j < i; j++) {
                object_type *j_ptr;
                j_ptr = &owner_ptr->inventory_list[j];
                if (!j_ptr->k_idx)
                    continue;

                /*
                 * Get maximum number of the stack if these
                 * are similar, get zero otherwise.
                 */
                int max_num = object_similar_part(j_ptr, o_ptr);

                bool is_max = (max_num != 0) && (j_ptr->number < max_num);
                if (!is_max)
                    continue;

                if (o_ptr->number + j_ptr->number <= max_num) {
                    flag = true;
                    object_absorb(j_ptr, o_ptr);
                    owner_ptr->inven_cnt--;
                    int k;
                    for (k = i; k < INVEN_PACK; k++) {
                        owner_ptr->inventory_list[k] = owner_ptr->inventory_list[k + 1];
                    }

                    (&owner_ptr->inventory_list[k])->wipe();
                } else {
                    int old_num = o_ptr->number;
                    int remain = j_ptr->number + o_ptr->number - max_num;
                    object_absorb(j_ptr, o_ptr);
                    o_ptr->number = remain;
                    if (o_ptr->tval == TV_ROD) {
                        o_ptr->pval = o_ptr->pval * remain / old_num;
                        o_ptr->timeout = o_ptr->timeout * remain / old_num;
                    }

                    if (o_ptr->tval == TV_WAND) {
                        o_ptr->pval = o_ptr->pval * remain / old_num;
                    }
                }

                owner_ptr->window_flags |= (PW_INVEN);
                combined = true;
                break;
            }
        }
    }

    if (flag)
        msg_print(_("ザックの中のアイテムをまとめ直した。", "You combine some items in your pack."));
}

/*!
 * @brief プレイヤーの所持スロットに存在するオブジェクトを並び替える /
 * Reorder items in the pack
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @details
 * Note special handling of the "overflow" slot
 */
void reorder_pack(player_type *owner_ptr)
{
    int i, j, k;
    int32_t o_value;
    object_type forge;
    object_type *q_ptr;
    object_type *o_ptr;
    bool flag = false;

    for (i = 0; i < INVEN_PACK; i++) {
        if ((i == INVEN_PACK) && (owner_ptr->inven_cnt == INVEN_PACK))
            break;

        o_ptr = &owner_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        o_value = object_value(owner_ptr, o_ptr);
        for (j = 0; j < INVEN_PACK; j++) {
            if (object_sort_comp(owner_ptr, o_ptr, o_value, &owner_ptr->inventory_list[j]))
                break;
        }

        if (j >= i)
            continue;

        flag = true;
        q_ptr = &forge;
        q_ptr->copy_from(&owner_ptr->inventory_list[i]);
        for (k = i; k > j; k--) {
            (&owner_ptr->inventory_list[k])->copy_from(&owner_ptr->inventory_list[k - 1]);
        }

        (&owner_ptr->inventory_list[j])->copy_from(q_ptr);
        owner_ptr->window_flags |= (PW_INVEN);
    }

    if (flag)
        msg_print(_("ザックの中のアイテムを並べ直した。", "You reorder some items in your pack."));
}

/*!
 * @brief オブジェクトをプレイヤーが拾って所持スロットに納めるメインルーチン /
 * Add an item to the players inventory, and return the slot used.
 * @param o_ptr 拾うオブジェクトの構造体参照ポインタ
 * @return 収められた所持スロットのID、拾うことができなかった場合-1を返す。
 * @details
 * If the new item can combine with an existing item in the inventory,\n
 * it will do so, using "object_similar()" and "object_absorb()", else,\n
 * the item will be placed into the "proper" location in the inventory.\n
 *\n
 * This function can be used to "over-fill" the player's pack, but only\n
 * once, and such an action must trigger the "overflow" code immediately.\n
 * Note that when the pack is being "over-filled", the new item must be\n
 * placed into the "overflow" slot, and the "overflow" must take place\n
 * before the pack is reordered, but (optionally) after the pack is\n
 * combined.  This may be tricky.  See "dungeon.c" for info.\n
 *\n
 * Note that this code must remove any location/stack information\n
 * from the object once it is placed into the inventory.\n
 */
int16_t store_item_to_inventory(player_type *owner_ptr, object_type *o_ptr)
{
    INVENTORY_IDX i, j, k;
    INVENTORY_IDX n = -1;

    object_type *j_ptr;
    for (j = 0; j < INVEN_PACK; j++) {
        j_ptr = &owner_ptr->inventory_list[j];
        if (!j_ptr->k_idx)
            continue;

        n = j;
        if (object_similar(j_ptr, o_ptr)) {
            object_absorb(j_ptr, o_ptr);

            owner_ptr->update |= (PU_BONUS);
            owner_ptr->window_flags |= (PW_INVEN | PW_PLAYER);
            return (j);
        }
    }

    if (owner_ptr->inven_cnt > INVEN_PACK)
        return -1;

    for (j = 0; j <= INVEN_PACK; j++) {
        j_ptr = &owner_ptr->inventory_list[j];
        if (!j_ptr->k_idx)
            break;
    }

    i = j;
    if (i < INVEN_PACK) {
        int32_t o_value = object_value(owner_ptr, o_ptr);
        for (j = 0; j < INVEN_PACK; j++) {
            if (object_sort_comp(owner_ptr, o_ptr, o_value, &owner_ptr->inventory_list[j]))
                break;
        }

        i = j;
        for (k = n; k >= i; k--) {
            (&owner_ptr->inventory_list[k + 1])->copy_from(&owner_ptr->inventory_list[k]);
        }

        (&owner_ptr->inventory_list[i])->wipe();
    }

    (&owner_ptr->inventory_list[i])->copy_from(o_ptr);
    j_ptr = &owner_ptr->inventory_list[i];
    j_ptr->held_m_idx = 0;
    j_ptr->iy = j_ptr->ix = 0;
    j_ptr->marked = OM_TOUCHED;

    owner_ptr->inven_cnt++;
    owner_ptr->update |= (PU_BONUS | PU_COMBINE | PU_REORDER);
    owner_ptr->window_flags |= (PW_INVEN | PW_PLAYER);

    return i;
}

/*!
 * @brief アイテムを拾う際にザックから溢れずに済むかを判定する /
 * Check if we have space for an item in the pack without overflow
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 拾いたいオブジェクトの構造体参照ポインタ
 * @return 溢れずに済むならTRUEを返す
 */
bool check_store_item_to_inventory(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    if (player_ptr->inven_cnt < INVEN_PACK)
        return true;

    for (int j = 0; j < INVEN_PACK; j++) {
        object_type *j_ptr = &player_ptr->inventory_list[j];
        if (!j_ptr->k_idx)
            continue;

        if (object_similar(j_ptr, o_ptr))
            return true;
    }

    return false;
}

/*!
 * @brief 装備スロットからオブジェクトを外すメインルーチン /
 * Take off (some of) a non-cursed equipment item
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param item オブジェクトを外したい所持テーブルのID
 * @param amt 外したい個数
 * @return 収められた所持スロットのID、拾うことができなかった場合-1を返す。
 * @details
 * Note that only one item at a time can be wielded per slot.\n
 * Note that taking off an item when "full" may cause that item\n
 * to fall to the ground.\n
 * Return the inventory slot into which the item is placed.\n
 */
INVENTORY_IDX inven_takeoff(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER amt)
{
    INVENTORY_IDX slot;
    object_type forge;
    object_type *q_ptr;
    object_type *o_ptr;
    concptr act;
    GAME_TEXT o_name[MAX_NLEN];
    o_ptr = &owner_ptr->inventory_list[item];
    if (amt <= 0)
        return -1;

    if (amt > o_ptr->number)
        amt = o_ptr->number;
    q_ptr = &forge;
    q_ptr->copy_from(o_ptr);
    q_ptr->number = amt;
    describe_flavor(owner_ptr, o_name, q_ptr, 0);
    if (((item == INVEN_MAIN_HAND) || (item == INVEN_SUB_HAND)) && object_is_melee_weapon(o_ptr)) {
        act = _("を装備からはずした", "You were wielding");
    } else if (item == INVEN_BOW) {
        act = _("を装備からはずした", "You were holding");
    } else if (item == INVEN_LITE) {
        act = _("を光源からはずした", "You were holding");
    } else {
        act = _("を装備からはずした", "You were wearing");
    }

    inven_item_increase(owner_ptr, item, -amt);
    inven_item_optimize(owner_ptr, item);

    slot = store_item_to_inventory(owner_ptr, q_ptr);
#ifdef JP
    msg_format("%s(%c)%s。", o_name, index_to_label(slot), act);
#else
    msg_format("%s %s (%c).", act, o_name, index_to_label(slot));
#endif

    return slot;
}
