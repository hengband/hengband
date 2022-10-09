#include "market/building-recharger.h"
#include "autopick/autopick.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "inventory/inventory-slot-types.h"
#include "market/building-util.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-magic.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "perception/object-perception.h"
#include "spell-kind/spells-perception.h"
#include "system/baseitem-info-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief 魔道具の使用回数を回復させる施設のメインルーチン / Recharge rods, wands and staffs
 * @details
 * The player can select the number of charges to add\n
 * (up to a limit), and the recharge never fails.\n
 *\n
 * The cost for rods depends on the level of the rod. The prices\n
 * for recharging wands and staffs are dependent on the cost of\n
 * the base-item.\n
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void building_recharge(PlayerType *player_ptr)
{
    msg_flag = false;
    clear_bldg(4, 18);
    prt(_("  再充填の費用はアイテムの種類によります。", "  The prices of recharge depend on the type."), 6, 0);

    concptr q = _("どのアイテムに魔力を充填しますか? ", "Recharge which item? ");
    concptr s = _("魔力を充填すべきアイテムがない。", "You have nothing to recharge.");

    OBJECT_IDX item;
    ObjectType *o_ptr;
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(&ObjectType::is_rechargeable));
    if (!o_ptr) {
        return;
    }

    BaseItemInfo *k_ptr;
    k_ptr = &k_info[o_ptr->k_idx];

    /*
     * We don't want to give the player free info about
     * the level of the item or the number of charges.
     */
    /* The item must be "known" */
    char tmp_str[MAX_NLEN];
    if (!o_ptr->is_known()) {
        msg_format(_("充填する前に鑑定されている必要があります！", "The item must be identified first!"));
        msg_print(nullptr);

        if ((player_ptr->au >= 50) && get_check(_("＄50で鑑定しますか？ ", "Identify for 50 gold? ")))

        {
            player_ptr->au -= 50;
            identify_item(player_ptr, o_ptr);
            describe_flavor(player_ptr, tmp_str, o_ptr, 0);
            msg_format(_("%s です。", "You have: %s."), tmp_str);

            autopick_alter_item(player_ptr, item, false);
            building_prt_gold(player_ptr);
        }

        return;
    }

    DEPTH lev = k_info[o_ptr->k_idx].level;
    PRICE price;
    if (o_ptr->tval == ItemKindType::ROD) {
        if (o_ptr->timeout > 0) {
            price = (lev * 50 * o_ptr->timeout) / k_ptr->pval;
        } else {
            price = 0;
            msg_format(_("それは再充填する必要はありません。", "That doesn't need to be recharged."));
            return;
        }
    } else if (o_ptr->tval == ItemKindType::STAFF) {
        price = (k_info[o_ptr->k_idx].cost / 10) * o_ptr->number;
        price = std::max(10, price);
    } else {
        price = (k_info[o_ptr->k_idx].cost / 10);
        price = std::max(10, price);
    }

    if (o_ptr->tval == ItemKindType::WAND && (o_ptr->pval / o_ptr->number >= k_ptr->pval)) {
        if (o_ptr->number > 1) {
            msg_print(_("この魔法棒はもう充分に充填されています。", "These wands are already fully charged."));
        } else {
            msg_print(_("この魔法棒はもう充分に充填されています。", "This wand is already fully charged."));
        }

        return;
    } else if (o_ptr->tval == ItemKindType::STAFF && o_ptr->pval >= k_ptr->pval) {
        if (o_ptr->number > 1) {
            msg_print(_("この杖はもう充分に充填されています。", "These staffs are already fully charged."));
        } else {
            msg_print(_("この杖はもう充分に充填されています。", "This staff is already fully charged."));
        }

        return;
    }

    if (player_ptr->au < price) {
        describe_flavor(player_ptr, tmp_str, o_ptr, OD_NAME_ONLY);
#ifdef JP
        msg_format("%sを再充填するには＄%d 必要です！", tmp_str, price);
#else
        msg_format("You need %d gold to recharge %s!", price, tmp_str);
#endif
        return;
    }

    PARAMETER_VALUE charges;
    if (o_ptr->tval == ItemKindType::ROD) {
#ifdef JP
        if (get_check(format("そのロッドを＄%d で再充填しますか？", price)))
#else
        if (get_check(format("Recharge the %s for %d gold? ", ((o_ptr->number > 1) ? "rods" : "rod"), price)))
#endif

        {
            o_ptr->timeout = 0;
        } else {
            return;
        }
    } else {
        int max_charges;
        if (o_ptr->tval == ItemKindType::STAFF) {
            max_charges = k_ptr->pval - o_ptr->pval;
        } else {
            max_charges = o_ptr->number * k_ptr->pval - o_ptr->pval;
        }

        charges = (PARAMETER_VALUE)get_quantity(
            format(_("一回分＄%d で何回分充填しますか？", "Add how many charges for %d gold apiece? "), price), std::min(player_ptr->au / price, max_charges));

        if (charges < 1) {
            return;
        }

        price *= charges;
        o_ptr->pval += charges;
        o_ptr->ident &= ~(IDENT_EMPTY);
    }

    describe_flavor(player_ptr, tmp_str, o_ptr, 0);
#ifdef JP
    msg_format("%sを＄%d で再充填しました。", tmp_str, price);
#else
    msg_format("%^s %s recharged for %d gold.", tmp_str, ((o_ptr->number > 1) ? "were" : "was"), price);
#endif
    player_ptr->update |= (PU_COMBINE | PU_REORDER);
    player_ptr->window_flags |= (PW_INVEN);
    player_ptr->au -= price;
}

/*!
 * @brief 魔道具の使用回数を回復させる施設の一括処理向けサブルーチン / Recharge rods, wands and staffs
 * @details
 * The player can select the number of charges to add\n
 * (up to a limit), and the recharge never fails.\n
 *\n
 * The cost for rods depends on the level of the rod. The prices\n
 * for recharging wands and staffs are dependent on the cost of\n
 * the base-item.\n
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void building_recharge_all(PlayerType *player_ptr)
{
    msg_flag = false;
    clear_bldg(4, 18);
    prt(_("  再充填の費用はアイテムの種類によります。", "  The prices of recharge depend on the type."), 6, 0);

    PRICE price = 0;
    PRICE total_cost = 0;
    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        ObjectType *o_ptr;
        o_ptr = &player_ptr->inventory_list[i];

        if ((o_ptr->tval < ItemKindType::STAFF) || (o_ptr->tval > ItemKindType::ROD)) {
            continue;
        }
        if (!o_ptr->is_known()) {
            total_cost += 50;
        }

        DEPTH lev = k_info[o_ptr->k_idx].level;
        BaseItemInfo *k_ptr;
        k_ptr = &k_info[o_ptr->k_idx];

        switch (o_ptr->tval) {
        case ItemKindType::ROD:
            price = (lev * 50 * o_ptr->timeout) / k_ptr->pval;
            break;

        case ItemKindType::STAFF:
            price = (k_info[o_ptr->k_idx].cost / 10) * o_ptr->number;
            price = std::max(10, price);
            price = (k_ptr->pval - o_ptr->pval) * price;
            break;

        case ItemKindType::WAND:
            price = (k_info[o_ptr->k_idx].cost / 10);
            price = std::max(10, price);
            price = (o_ptr->number * k_ptr->pval - o_ptr->pval) * price;
            break;

        default:
            break;
        }

        if (price > 0) {
            total_cost += price;
        }
    }

    if (!total_cost) {
        msg_print(_("充填する必要はありません。", "No need to recharge."));
        msg_print(nullptr);
        return;
    }

    if (player_ptr->au < total_cost) {
        msg_format(_("すべてのアイテムを再充填するには＄%d 必要です！", "You need %d gold to recharge all items!"), total_cost);
        msg_print(nullptr);
        return;
    }

    if (!get_check(format(_("すべてのアイテムを ＄%d で再充填しますか？", "Recharge all items for %d gold? "), total_cost))) {
        return;
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        ObjectType *o_ptr;
        o_ptr = &player_ptr->inventory_list[i];
        BaseItemInfo *k_ptr;
        k_ptr = &k_info[o_ptr->k_idx];

        if ((o_ptr->tval < ItemKindType::STAFF) || (o_ptr->tval > ItemKindType::ROD)) {
            continue;
        }

        if (!o_ptr->is_known()) {
            identify_item(player_ptr, o_ptr);
            autopick_alter_item(player_ptr, i, false);
        }

        switch (o_ptr->tval) {
        case ItemKindType::ROD:
            o_ptr->timeout = 0;
            break;
        case ItemKindType::STAFF:
            if (o_ptr->pval < k_ptr->pval) {
                o_ptr->pval = k_ptr->pval;
            }

            o_ptr->ident &= ~(IDENT_EMPTY);
            break;
        case ItemKindType::WAND:
            if (o_ptr->pval < o_ptr->number * k_ptr->pval) {
                o_ptr->pval = o_ptr->number * k_ptr->pval;
            }

            o_ptr->ident &= ~(IDENT_EMPTY);
            break;

        default:
            break;
        }
    }

    msg_format(_("＄%d で再充填しました。", "You pay %d gold."), total_cost);
    msg_print(nullptr);
    player_ptr->update |= (PU_COMBINE | PU_REORDER);
    player_ptr->window_flags |= (PW_INVEN);
    player_ptr->au -= total_cost;
}
