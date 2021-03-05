﻿/*!
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
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-magic.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-generator.h"
#include "object/object-kind.h"
#include "player/player-realm.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 魔力食い処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param power 基本効力
 * @return ターンを消費した場合TRUEを返す
 */
bool eat_magic(player_type *caster_ptr, int power)
{
    byte fail_type = 1;
    GAME_TEXT o_name[MAX_NLEN];

    item_tester_hook = item_tester_hook_recharge;

    concptr q = _("どのアイテムから魔力を吸収しますか？", "Drain which item? ");
    concptr s = _("魔力を吸収できるアイテムがありません。", "You have nothing to drain.");

    object_type *o_ptr;
    OBJECT_IDX item;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), TV_NONE);
    if (!o_ptr)
        return FALSE;

    object_kind *k_ptr;
    k_ptr = &k_info[o_ptr->k_idx];
    DEPTH lev = k_info[o_ptr->k_idx].level;

    int recharge_strength = 0;
    bool is_eating_successful = TRUE;
    if (o_ptr->tval == TV_ROD) {
        recharge_strength = ((power > lev / 2) ? (power - lev / 2) : 0) / 5;
        if (one_in_(recharge_strength)) {
            is_eating_successful = FALSE;
        } else {
            if (o_ptr->timeout > (o_ptr->number - 1) * k_ptr->pval) {
                msg_print(_("充填中のロッドから魔力を吸収することはできません。", "You can't absorb energy from a discharged rod."));
            } else {
                caster_ptr->csp += lev;
                o_ptr->timeout += k_ptr->pval;
            }
        }
    } else {
        recharge_strength = (100 + power - lev) / 15;
        if (recharge_strength < 0)
            recharge_strength = 0;

        if (one_in_(recharge_strength)) {
            is_eating_successful = FALSE;
        } else {
            if (o_ptr->pval > 0) {
                caster_ptr->csp += lev / 2;
                o_ptr->pval--;

                if ((o_ptr->tval == TV_STAFF) && (item >= 0) && (o_ptr->number > 1)) {
                    object_type forge;
                    object_type *q_ptr;
                    q_ptr = &forge;
                    object_copy(q_ptr, o_ptr);

                    q_ptr->number = 1;
                    o_ptr->pval++;
                    o_ptr->number--;
                    item = store_item_to_inventory(caster_ptr, q_ptr);

                    msg_print(_("杖をまとめなおした。", "You unstack your staff."));
                }
            } else {
                msg_print(_("吸収できる魔力がありません！", "There's no energy there to absorb!"));
            }

            if (!o_ptr->pval)
                o_ptr->ident |= IDENT_EMPTY;
        }
    }

    if (is_eating_successful) {
        return redraw_player(caster_ptr);
    }

    if (object_is_fixed_artifact(o_ptr)) {
        describe_flavor(caster_ptr, o_name, o_ptr, OD_NAME_ONLY);
        msg_format(_("魔力が逆流した！%sは完全に魔力を失った。", "The recharging backfires - %s is completely drained!"), o_name);
        if (o_ptr->tval == TV_ROD)
            o_ptr->timeout = k_ptr->pval * o_ptr->number;
        else if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF))
            o_ptr->pval = 0;

        return redraw_player(caster_ptr);
    }

    describe_flavor(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    /* Mages recharge objects more safely. */
    if (is_wizard_class(caster_ptr)) {
        /* 10% chance to blow up one rod, otherwise draining. */
        if (o_ptr->tval == TV_ROD) {
            if (one_in_(10))
                fail_type = 2;
            else
                fail_type = 1;
        }
        /* 75% chance to blow up one wand, otherwise draining. */
        else if (o_ptr->tval == TV_WAND) {
            if (!one_in_(3))
                fail_type = 2;
            else
                fail_type = 1;
        }
        /* 50% chance to blow up one staff, otherwise no effect. */
        else if (o_ptr->tval == TV_STAFF) {
            if (one_in_(2))
                fail_type = 2;
            else
                fail_type = 0;
        }
    }

    /* All other classes get no special favors. */
    else {
        /* 33% chance to blow up one rod, otherwise draining. */
        if (o_ptr->tval == TV_ROD) {
            if (one_in_(3))
                fail_type = 2;
            else
                fail_type = 1;
        }
        /* 20% chance of the entire stack, else destroy one wand. */
        else if (o_ptr->tval == TV_WAND) {
            if (one_in_(5))
                fail_type = 3;
            else
                fail_type = 2;
        }
        /* Blow up one staff. */
        else if (o_ptr->tval == TV_STAFF) {
            fail_type = 2;
        }
    }

    if (fail_type == 1) {
        if (o_ptr->tval == TV_ROD) {
            msg_format(_("ロッドは破損を免れたが、魔力は全て失なわれた。", "You save your rod from destruction, but all charges are lost."), o_name);
            o_ptr->timeout = k_ptr->pval * o_ptr->number;
        } else if (o_ptr->tval == TV_WAND) {
            msg_format(_("%sは破損を免れたが、魔力が全て失われた。", "You save your %s from destruction, but all charges are lost."), o_name);
            o_ptr->pval = 0;
        }
    }

    if (fail_type == 2) {
        if (o_ptr->number > 1) {
            msg_format(_("乱暴な魔法のために%sが一本壊れた！", "Wild magic consumes one of your %s!"), o_name);
            /* Reduce rod stack maximum timeout, drain wands. */
            if (o_ptr->tval == TV_ROD)
                o_ptr->timeout = MIN(o_ptr->timeout, k_ptr->pval * (o_ptr->number - 1));
            else if (o_ptr->tval == TV_WAND)
                o_ptr->pval = o_ptr->pval * (o_ptr->number - 1) / o_ptr->number;
        } else {
            msg_format(_("乱暴な魔法のために%sが何本か壊れた！", "Wild magic consumes your %s!"), o_name);
        }

        vary_item(caster_ptr, item, -1);
    }

    if (fail_type == 3) {
        if (o_ptr->number > 1)
            msg_format(_("乱暴な魔法のために%sが全て壊れた！", "Wild magic consumes all your %s!"), o_name);
        else
            msg_format(_("乱暴な魔法のために%sが壊れた！", "Wild magic consumes your %s!"), o_name);

        vary_item(caster_ptr, item, -999);
    }

    return redraw_player(caster_ptr);
}
