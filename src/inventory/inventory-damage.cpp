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
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

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
    INVENTORY_IDX i;
    int j, amt;
    ItemEntity *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];

    if (check_multishadow(player_ptr) || player_ptr->current_floor_ptr->inside_arena) {
        return;
    }

    /* Scan through the slots backwards */
    for (i = 0; i < INVEN_PACK; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx) {
            continue;
        }

        /* Hack -- for now, skip artifacts */
        if (o_ptr->is_artifact()) {
            continue;
        }

        /* Give this item slot a shot at death */
        if (!breaker.can_destroy(o_ptr)) {
            continue;
        }

        /* Count the casualties */
        for (amt = j = 0; j < o_ptr->number; ++j) {
            if (randint0(100) < perc) {
                amt++;
            }
        }

        /* Some casualities */
        if (!amt) {
            continue;
        }

        describe_flavor(player_ptr, o_name, o_ptr, OD_OMIT_PREFIX);

        msg_format(_("%s(%c)が%s壊れてしまった！", "%sour %s (%c) %s destroyed!"),
#ifdef JP
            o_name, index_to_label(i), ((o_ptr->number > 1) ? ((amt == o_ptr->number) ? "全部" : (amt > 1 ? "何個か" : "一個")) : ""));
#else
            ((o_ptr->number > 1) ? ((amt == o_ptr->number) ? "All of y" : (amt > 1 ? "Some of y" : "One of y")) : "Y"), o_name, index_to_label(i),
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

        /* Potions smash open */
        if (o_ptr->is_potion()) {
            (void)potion_smash_effect(player_ptr, 0, player_ptr->y, player_ptr->x, o_ptr->k_idx);
        }

        /* Reduce the charges of rods/wands */
        reduce_charges(o_ptr, amt);

        /* Destroy "amt" items */
        inven_item_increase(player_ptr, i, -amt);
        inven_item_optimize(player_ptr, i);
    }
}
