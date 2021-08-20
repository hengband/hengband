﻿/*!
 * @file weapon-shield.cpp
 * @brief 手装備持ち替え処理実装
 */

#include "action/weapon-shield.h"
#include "flavor/flavor-describer.h"
#include "game-option/birth-options.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-weapon.h"
#include "player-info/equipment-info.h"
#include "player-status/player-hand-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 持ち替え処理
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param item 持ち替えを行いたい装備部位ID
 */
void verify_equip_slot(player_type *owner_ptr, INVENTORY_IDX item)
{
    object_type *o_ptr, *new_o_ptr;
    GAME_TEXT o_name[MAX_NLEN];

    if (item == INVEN_MAIN_HAND) {
        if (!has_melee_weapon(owner_ptr, INVEN_SUB_HAND))
            return;

        o_ptr = &owner_ptr->inventory_list[INVEN_SUB_HAND];
        describe_flavor(owner_ptr, o_name, o_ptr, 0);

        if (object_is_cursed(o_ptr)) {
            if (object_allow_two_hands_wielding(o_ptr) && can_two_hands_wielding(owner_ptr))
                msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), o_name);
            return;
        }

        new_o_ptr = &owner_ptr->inventory_list[INVEN_MAIN_HAND];
        new_o_ptr->copy_from(o_ptr);
        inven_item_increase(owner_ptr, INVEN_SUB_HAND, -(static_cast<int>(o_ptr->number)));
        inven_item_optimize(owner_ptr, INVEN_SUB_HAND);
        if (object_allow_two_hands_wielding(o_ptr) && can_two_hands_wielding(owner_ptr))
            msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), o_name);
        else
            msg_format(_("%sを%sで構えた。", "You are wielding %s in your %s hand."), o_name, (left_hander ? _("左手", "left") : _("右手", "right")));
        return;
    }

    if (item != INVEN_SUB_HAND)
        return;

    o_ptr = &owner_ptr->inventory_list[INVEN_MAIN_HAND];
    if (o_ptr->k_idx)
        describe_flavor(owner_ptr, o_name, o_ptr, 0);

    if (has_melee_weapon(owner_ptr, INVEN_MAIN_HAND)) {
        if (object_allow_two_hands_wielding(o_ptr) && can_two_hands_wielding(owner_ptr))
            msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), o_name);

        return;
    }

    if ((empty_hands(owner_ptr, false) & EMPTY_HAND_MAIN) || object_is_cursed(o_ptr))
        return;

    new_o_ptr = &owner_ptr->inventory_list[INVEN_SUB_HAND];
    new_o_ptr->copy_from(o_ptr);
    inven_item_increase(owner_ptr, INVEN_MAIN_HAND, -(static_cast<int>(o_ptr->number)));
    inven_item_optimize(owner_ptr, INVEN_MAIN_HAND);
    msg_format(_("%sを持ち替えた。", "You switched hand of %s."), o_name);
}
