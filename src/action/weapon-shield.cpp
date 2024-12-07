/*!
 * @file weapon-shield.cpp
 * @brief 手装備持ち替え処理実装
 */

#include "action/weapon-shield.h"
#include "flavor/flavor-describer.h"
#include "game-option/birth-options.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "object-hook/hook-weapon.h"
#include "player-info/equipment-info.h"
#include "player-status/player-hand-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 持ち替え処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i_idx 持ち替えを行いたい装備部位ID
 */
void verify_equip_slot(PlayerType *player_ptr, INVENTORY_IDX i_idx)
{
    std::string item_name;
    if (i_idx == INVEN_MAIN_HAND) {
        if (!has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
            return;
        }

        const auto &item_sub_hand = player_ptr->inventory_list[INVEN_SUB_HAND];
        item_name = describe_flavor(player_ptr, item_sub_hand, 0);

        if (item_sub_hand.is_cursed()) {
            if (item_sub_hand.allow_two_hands_wielding() && can_two_hands_wielding(player_ptr)) {
                msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), item_name.data());
            }
            return;
        }

        auto &item_main_hand = player_ptr->inventory_list[INVEN_MAIN_HAND];
        item_main_hand = item_sub_hand.clone();
        inven_item_increase(player_ptr, INVEN_SUB_HAND, -item_sub_hand.number);
        inven_item_optimize(player_ptr, INVEN_SUB_HAND);
        if (item_main_hand.allow_two_hands_wielding() && can_two_hands_wielding(player_ptr)) {
            msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), item_name.data());
        } else {
            const auto mes = _("%sを%sで構えた。", "You are wielding %s in your %s hand.");
            msg_format(mes, item_name.data(), (left_hander ? _("左手", "left") : _("右手", "right")));
        }
        return;
    }

    if (i_idx != INVEN_SUB_HAND) {
        return;
    }

    const auto &item_main_hand = player_ptr->inventory_list[INVEN_MAIN_HAND];
    if (item_main_hand.is_valid()) {
        item_name = describe_flavor(player_ptr, item_main_hand, 0);
    }

    if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND)) {
        if (item_main_hand.allow_two_hands_wielding() && can_two_hands_wielding(player_ptr)) {
            msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), item_name.data());
        }

        return;
    }

    if ((empty_hands(player_ptr, false) & EMPTY_HAND_MAIN) || item_main_hand.is_cursed()) {
        return;
    }

    auto &item_sub_hand = player_ptr->inventory_list[INVEN_SUB_HAND];
    item_sub_hand = item_main_hand.clone();
    inven_item_increase(player_ptr, INVEN_MAIN_HAND, -item_main_hand.number);
    inven_item_optimize(player_ptr, INVEN_MAIN_HAND);
    msg_format(_("%sを持ち替えた。", "You shifted %s to your other hand."), item_name.data());
}
