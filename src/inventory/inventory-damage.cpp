#include "inventory/inventory-damage.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-mirror-master.h"
#include "object-hook/hook-expendable.h"
#include "object/object-broken.h"
#include "object/object-info.h"
#include "object/object-stack.h"
#include "player/player-status.h"
#include "system/floor/floor-info.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include <vector>

/*!
 * @brief 手持ちのアイテムを指定確率で破損させる /
 * Destroys a type of item on a given percent chance
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param typ 破損判定関数ポインタ
 * @param perc 基本確率
 * @details
 * Note that missiles are no longer necessarily all destroyed
 * Destruction taken from "melee.c" code for "stealing".
 * New-style wands and rods handled correctly. -LM-
 */
void inventory_damage(PlayerType *player_ptr, const ObjectBreaker &breaker, int perc)
{
    int j, amt;
    if (check_multishadow(player_ptr) || player_ptr->current_floor_ptr->inside_arena) {
        return;
    }

    std::vector<short> smashed_potions;

    /* Scan through the slots backwards */
    for (const auto i_idx : INVEN_PACK_SLOTS) {
        auto &item = *player_ptr->inventory[i_idx];
        if (!item.is_valid()) {
            continue;
        }

        /* Hack -- for now, skip artifacts */
        if (item.is_fixed_or_random_artifact()) {
            continue;
        }

        /* Give this item slot a shot at death */
        if (!breaker.can_destroy(&item)) {
            continue;
        }

        /* Count the casualties */
        for (amt = j = 0; j < item.number; ++j) {
            if (evaluate_percent(perc)) {
                amt++;
            }
        }

        /* Some casualities */
        if (!amt) {
            continue;
        }

        const auto item_name = describe_flavor(player_ptr, item, OD_OMIT_PREFIX);

        msg_format(_("%s(%c)が%s壊れてしまった！", "%sour %s (%c) %s destroyed!"),
#ifdef JP
            item_name.data(), index_to_label(i_idx), ((item.number > 1) ? ((amt == item.number) ? "全部" : (amt > 1 ? "何個か" : "一個")) : ""));
#else
            ((item.number > 1) ? ((amt == item.number) ? "All of y" : (amt > 1 ? "Some of y" : "One of y")) : "Y"), item_name.data(), index_to_label(i_idx),
            ((amt > 1) ? "were" : "was"));
#endif

#ifdef JP
        if (is_echizen(player_ptr)) {
            msg_print("やりやがったな！");
        } else if (is_chargeman(player_ptr)) {
            if (randint0(2) == 0) {
                msg_print(_("ジュラル星人め！", ""));
            } else {
                msg_print(_("弱い者いじめは止めるんだ！", ""));
            }
        }
#endif

        if (item.is_potion()) {
            smashed_potions.push_back(item.bi_id);
        }

        /* Reduce the charges of rods/wands */
        reduce_charges(&item, amt);

        /* Destroy "amt" items */
        inven_item_increase(player_ptr, i_idx, -amt);
        inven_item_optimize(player_ptr, i_idx);
    }

    // 薬の効果の最中に所持品の結合やモンスターの爆発による再入が起きると所持品の番号がずれるため、
    // 割れた薬の効果は所持品を減らし終えてから起こす。
    for (const auto bi_id : smashed_potions) {
        (void)potion_smash_effect(player_ptr, 0, player_ptr->y, player_ptr->x, bi_id);
    }
}
