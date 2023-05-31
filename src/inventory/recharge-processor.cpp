#include "inventory/recharge-processor.h"
#include "core/disturbance.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "hpmp/hp-mp-regenerator.h"
#include "inventory/inventory-slot-types.h"
#include "object/tval-types.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief
 * !!を刻んだ魔道具の時間経過による再充填を知らせる処理 /
 * If player has inscribed the object with "!!", let him know when it's recharged. -LM-
 * @param o_ptr 対象オブジェクトの構造体参照ポインタ
 */
static void recharged_notice(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    if (!o_ptr->is_inscribed()) {
        return;
    }

    auto s = angband_strchr(o_ptr->inscription->data(), '!');
    while (s) {
        if (s[1] == '!') {
            const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
            msg_format("%sは再充填された。", item_name.data());
#else
            if (o_ptr->number > 1) {
                msg_format("Your %s are recharged.", item_name.data());
            } else {
                msg_format("Your %s is recharged.", item_name.data());
            }
#endif
            disturb(player_ptr, false, false);
            return;
        }

        s = angband_strchr(s + 1, '!');
    }
}

/*!
 * @brief 10ゲームターンが進行するごとに魔道具の自然充填を行う処理
 * / Handle recharging objects once every 10 game turns
 */
void recharge_magic_items(PlayerType *player_ptr)
{
    int i;
    bool changed;

    for (changed = false, i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->timeout > 0) {
            o_ptr->timeout--;
            if (!o_ptr->timeout) {
                recharged_notice(player_ptr, o_ptr);
                changed = true;
            }
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (changed) {
        rfu.set_flag(SubWindowRedrawingFlag::EQUIPMENT);
        wild_regen = 20;
    }

    /*
     * Recharge rods.  Rods now use timeout to control charging status,
     * and each charging rod in a stack decreases the stack's timeout by
     * one per turn. -LM-
     */
    for (changed = false, i = 0; i < INVEN_PACK; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        const auto &baseitem = o_ptr->get_baseitem();
        if (!o_ptr->is_valid()) {
            continue;
        }

        if ((o_ptr->bi_key.tval() == ItemKindType::ROD) && (o_ptr->timeout)) {
            TIME_EFFECT temp = (o_ptr->timeout + (baseitem.pval - 1)) / baseitem.pval;
            if (temp > o_ptr->number) {
                temp = (TIME_EFFECT)o_ptr->number;
            }

            o_ptr->timeout -= temp;
            if (o_ptr->timeout < 0) {
                o_ptr->timeout = 0;
            }

            if (!(o_ptr->timeout)) {
                recharged_notice(player_ptr, o_ptr);
                changed = true;
            } else if (o_ptr->timeout % baseitem.pval) {
                changed = true;
            }
        }
    }

    if (changed) {
        rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
        wild_regen = 20;
    }

    for (i = 1; i < player_ptr->current_floor_ptr->o_max; i++) {
        auto *o_ptr = &player_ptr->current_floor_ptr->o_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        if ((o_ptr->bi_key.tval() == ItemKindType::ROD) && (o_ptr->timeout)) {
            o_ptr->timeout -= (TIME_EFFECT)o_ptr->number;
            if (o_ptr->timeout < 0) {
                o_ptr->timeout = 0;
            }
        }
    }
}
