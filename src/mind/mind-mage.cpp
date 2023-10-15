/*!
 * @brief 魔力喰い処理
 * @date 2020/06/27
 * @author Hourier
 */

#include "mind/mind-mage.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "inventory/inventory-object.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-magic.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-base/player-class.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 魔力食い処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param power 基本効力
 * @return ターンを消費した場合TRUEを返す
 */
bool eat_magic(PlayerType *player_ptr, int power)
{
    byte fail_type = 1;
    constexpr auto q = _("どのアイテムから魔力を吸収しますか？", "Drain which item? ");
    constexpr auto s = _("魔力を吸収できるアイテムがありません。", "You have nothing to drain.");
    short i_idx;
    auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(&ItemEntity::can_recharge));
    if (o_ptr == nullptr) {
        return false;
    }

    const auto &baseitem = o_ptr->get_baseitem();
    const auto lev = baseitem.level;
    const auto tval = o_ptr->bi_key.tval();
    auto recharge_strength = 0;
    auto is_eating_successful = true;
    if (tval == ItemKindType::ROD) {
        recharge_strength = ((power > lev / 2) ? (power - lev / 2) : 0) / 5;
        if (one_in_(recharge_strength)) {
            is_eating_successful = false;
        } else {
            if (o_ptr->timeout > (o_ptr->number - 1) * baseitem.pval) {
                msg_print(_("充填中のロッドから魔力を吸収することはできません。", "You can't absorb energy from a discharged rod."));
            } else {
                player_ptr->csp += lev;
                o_ptr->timeout += baseitem.pval;
            }
        }
    } else {
        recharge_strength = (100 + power - lev) / 15;
        if (recharge_strength < 0) {
            recharge_strength = 0;
        }

        if (one_in_(recharge_strength)) {
            is_eating_successful = false;
        } else {
            if (o_ptr->pval > 0) {
                player_ptr->csp += lev / 2;
                o_ptr->pval--;

                if ((tval == ItemKindType::STAFF) && (i_idx >= 0) && (o_ptr->number > 1)) {
                    ItemEntity forge;
                    ItemEntity *q_ptr;
                    q_ptr = &forge;
                    q_ptr->copy_from(o_ptr);

                    q_ptr->number = 1;
                    o_ptr->pval++;
                    o_ptr->number--;
                    i_idx = store_item_to_inventory(player_ptr, q_ptr);

                    msg_print(_("杖をまとめなおした。", "You unstack your staff."));
                }
            } else {
                msg_print(_("吸収できる魔力がありません！", "There's no energy there to absorb!"));
            }

            if (!o_ptr->pval) {
                o_ptr->ident |= IDENT_EMPTY;
            }
        }
    }

    if (is_eating_successful) {
        return redraw_player(player_ptr);
    }

    if (o_ptr->is_fixed_artifact()) {
        const auto item_name = describe_flavor(player_ptr, o_ptr, OD_NAME_ONLY);
        msg_format(_("魔力が逆流した！%sは完全に魔力を失った。", "The recharging backfires - %s is completely drained!"), item_name.data());
        if (tval == ItemKindType::ROD) {
            o_ptr->timeout = baseitem.pval * o_ptr->number;
        } else if (o_ptr->is_wand_staff()) {
            o_ptr->pval = 0;
        }

        return redraw_player(player_ptr);
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    /* Mages recharge objects more safely. */
    if (PlayerClass(player_ptr).is_wizard()) {
        /* 10% chance to blow up one rod, otherwise draining. */
        if (tval == ItemKindType::ROD) {
            if (one_in_(10)) {
                fail_type = 2;
            } else {
                fail_type = 1;
            }
        }
        /* 75% chance to blow up one wand, otherwise draining. */
        else if (tval == ItemKindType::WAND) {
            if (!one_in_(3)) {
                fail_type = 2;
            } else {
                fail_type = 1;
            }
        }
        /* 50% chance to blow up one staff, otherwise no effect. */
        else if (tval == ItemKindType::STAFF) {
            if (one_in_(2)) {
                fail_type = 2;
            } else {
                fail_type = 0;
            }
        }
    }

    /* All other classes get no special favors. */
    else {
        /* 33% chance to blow up one rod, otherwise draining. */
        if (tval == ItemKindType::ROD) {
            if (one_in_(3)) {
                fail_type = 2;
            } else {
                fail_type = 1;
            }
        }
        /* 20% chance of the entire stack, else destroy one wand. */
        else if (tval == ItemKindType::WAND) {
            if (one_in_(5)) {
                fail_type = 3;
            } else {
                fail_type = 2;
            }
        }
        /* Blow up one staff. */
        else if (tval == ItemKindType::STAFF) {
            fail_type = 2;
        }
    }

    if (fail_type == 1) {
        if (tval == ItemKindType::ROD) {
            msg_print(_("ロッドは破損を免れたが、魔力は全て失なわれた。", "You save your rod from destruction, but all charges are lost."));
            o_ptr->timeout = baseitem.pval * o_ptr->number;
        } else if (tval == ItemKindType::WAND) {
            constexpr auto mes = _("%sは破損を免れたが、魔力が全て失われた。", "You save your %s from destruction, but all charges are lost.");
            msg_format(mes, item_name.data());
            o_ptr->pval = 0;
        }
    }

    if (fail_type == 2) {
        if (o_ptr->number > 1) {
            msg_format(_("乱暴な魔法のために%sが一本壊れた！", "Wild magic consumes one of your %s!"), item_name.data());
            /* Reduce rod stack maximum timeout, drain wands. */
            if (tval == ItemKindType::ROD) {
                o_ptr->timeout = std::min<short>(o_ptr->timeout, baseitem.pval * (o_ptr->number - 1));
            } else if (tval == ItemKindType::WAND) {
                o_ptr->pval = o_ptr->pval * (o_ptr->number - 1) / o_ptr->number;
            }
        } else {
            msg_format(_("乱暴な魔法のために%sが壊れた！", "Wild magic consumes your %s!"), item_name.data());
        }

        vary_item(player_ptr, i_idx, -1);
    }

    if (fail_type == 3) {
        if (o_ptr->number > 1) {
            msg_format(_("乱暴な魔法のために%sが全て壊れた！", "Wild magic consumes all your %s!"), item_name.data());
        } else {
            msg_format(_("乱暴な魔法のために%sが壊れた！", "Wild magic consumes your %s!"), item_name.data());
        }

        vary_item(player_ptr, i_idx, -999);
    }

    return redraw_player(player_ptr);
}
