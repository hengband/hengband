/*!
 * @brief 魔法効果の実装/ Spell code (part 3)
 * @date 2014/07/26
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "spell-kind/magic-item-recharger.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "inventory/inventory-object.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-magic.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-kind.h"
#include "player-base/player-class.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 魔力充填処理 /
 * Recharge a wand/staff/rod from the pack or on the floor.
 * This function has been rewritten in Oangband and ZAngband.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param power 充填パワー
 * @return ターン消費を要する処理まで進んだらTRUEを返す
 *
 * Sorcery/Arcane -- Recharge  --> recharge(plev * 4)
 * Chaos -- Arcane Binding     --> recharge(90)
 *
 * Scroll of recharging        --> recharge(130)
 * Artifact activation/Thingol --> recharge(130)
 *
 * It is harder to recharge high level, and highly charged wands,
 * staffs, and rods.  The more wands in a stack, the more easily and
 * strongly they recharge.  Staffs, however, each get fewer charges if
 * stacked.
 *
 * Beware of "sliding index errors".
 */
bool recharge(PlayerType *player_ptr, int power)
{
    concptr q = _("どのアイテムに魔力を充填しますか? ", "Recharge which item? ");
    concptr s = _("魔力を充填すべきアイテムがない。", "You have nothing to recharge.");

    OBJECT_IDX item;
    ObjectType *o_ptr;
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(&ObjectType::is_rechargeable));
    if (!o_ptr) {
        return false;
    }

    object_kind *k_ptr;
    k_ptr = &k_info[o_ptr->k_idx];
    DEPTH lev = k_info[o_ptr->k_idx].level;

    TIME_EFFECT recharge_amount;
    int recharge_strength;
    bool is_recharge_successful = true;
    if (o_ptr->tval == ItemKindType::ROD) {
        recharge_strength = ((power > lev / 2) ? (power - lev / 2) : 0) / 5;
        if (one_in_(recharge_strength)) {
            is_recharge_successful = false;
        } else {
            recharge_amount = (power * damroll(3, 2));
            if (o_ptr->timeout > recharge_amount) {
                o_ptr->timeout -= recharge_amount;
            } else {
                o_ptr->timeout = 0;
            }
        }
    } else {
        if ((o_ptr->tval == ItemKindType::WAND) && (o_ptr->number > 1)) {
            recharge_strength = (100 + power - lev - (8 * o_ptr->pval / o_ptr->number)) / 15;
        } else {
            recharge_strength = (100 + power - lev - (8 * o_ptr->pval)) / 15;
        }

        if (recharge_strength < 0) {
            recharge_strength = 0;
        }

        if (one_in_(recharge_strength)) {
            is_recharge_successful = false;
        } else {
            recharge_amount = randint1(1 + k_ptr->pval / 2);
            if ((o_ptr->tval == ItemKindType::WAND) && (o_ptr->number > 1)) {
                recharge_amount += (randint1(recharge_amount * (o_ptr->number - 1))) / 2;
                if (recharge_amount < 1) {
                    recharge_amount = 1;
                }
                if (recharge_amount > 12) {
                    recharge_amount = 12;
                }
            }

            if ((o_ptr->tval == ItemKindType::STAFF) && (o_ptr->number > 1)) {
                recharge_amount /= (TIME_EFFECT)o_ptr->number;
                if (recharge_amount < 1) {
                    recharge_amount = 1;
                }
            }

            o_ptr->pval += recharge_amount;
            o_ptr->ident &= ~(IDENT_KNOWN);
            o_ptr->ident &= ~(IDENT_EMPTY);
        }
    }

    if (is_recharge_successful) {
        return update_player(player_ptr);
    }

    byte fail_type = 1;
    GAME_TEXT o_name[MAX_NLEN];
    if (o_ptr->is_fixed_artifact()) {
        describe_flavor(player_ptr, o_name, o_ptr, OD_NAME_ONLY);
        msg_format(_("魔力が逆流した！%sは完全に魔力を失った。", "The recharging backfires - %s is completely drained!"), o_name);
        if ((o_ptr->tval == ItemKindType::ROD) && (o_ptr->timeout < 10000)) {
            o_ptr->timeout = (o_ptr->timeout + 100) * 2;
        } else if ((o_ptr->tval == ItemKindType::WAND) || (o_ptr->tval == ItemKindType::STAFF)) {
            o_ptr->pval = 0;
        }
        return update_player(player_ptr);
    }

    describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    if (PlayerClass(player_ptr).is_wizard()) {
        /* 10% chance to blow up one rod, otherwise draining. */
        if (o_ptr->tval == ItemKindType::ROD) {
            if (one_in_(10)) {
                fail_type = 2;
            } else {
                fail_type = 1;
            }
        }
        /* 75% chance to blow up one wand, otherwise draining. */
        else if (o_ptr->tval == ItemKindType::WAND) {
            if (!one_in_(3)) {
                fail_type = 2;
            } else {
                fail_type = 1;
            }
        }
        /* 50% chance to blow up one staff, otherwise no effect. */
        else if (o_ptr->tval == ItemKindType::STAFF) {
            if (one_in_(2)) {
                fail_type = 2;
            } else {
                fail_type = 0;
            }
        }
    } else {
        /* 33% chance to blow up one rod, otherwise draining. */
        if (o_ptr->tval == ItemKindType::ROD) {
            if (one_in_(3)) {
                fail_type = 2;
            } else {
                fail_type = 1;
            }
        }
        /* 20% chance of the entire stack, else destroy one wand. */
        else if (o_ptr->tval == ItemKindType::WAND) {
            if (one_in_(5)) {
                fail_type = 3;
            } else {
                fail_type = 2;
            }
        }
        /* Blow up one staff. */
        else if (o_ptr->tval == ItemKindType::STAFF) {
            fail_type = 2;
        }
    }

    if (fail_type == 1) {
        if (o_ptr->tval == ItemKindType::ROD) {
            msg_print(_("魔力が逆噴射して、ロッドからさらに魔力を吸い取ってしまった！", "The recharge backfires, draining the rod further!"));

            if (o_ptr->timeout < 10000) {
                o_ptr->timeout = (o_ptr->timeout + 100) * 2;
            }
        } else if (o_ptr->tval == ItemKindType::WAND) {
            msg_format(_("%sは破損を免れたが、魔力が全て失われた。", "You save your %s from destruction, but all charges are lost."), o_name);
            o_ptr->pval = 0;
        }
    }

    if (fail_type == 2) {
        if (o_ptr->number > 1) {
            msg_format(_("乱暴な魔法のために%sが一本壊れた！", "Wild magic consumes one of your %s!"), o_name);
        } else {
            msg_format(_("乱暴な魔法のために%sが壊れた！", "Wild magic consumes your %s!"), o_name);
        }

        if (o_ptr->tval == ItemKindType::ROD) {
            o_ptr->timeout = (o_ptr->number - 1) * k_ptr->pval;
        }
        if (o_ptr->tval == ItemKindType::WAND) {
            o_ptr->pval = 0;
        }

        vary_item(player_ptr, item, -1);
    }

    if (fail_type == 3) {
        if (o_ptr->number > 1) {
            msg_format(_("乱暴な魔法のために%sが全て壊れた！", "Wild magic consumes all your %s!"), o_name);
        } else {
            msg_format(_("乱暴な魔法のために%sが壊れた！", "Wild magic consumes your %s!"), o_name);
        }

        vary_item(player_ptr, item, -999);
    }

    return update_player(player_ptr);
}
