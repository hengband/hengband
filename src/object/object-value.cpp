#include "object/object-value.h"
#include "monster-race/monster-race.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object/object-broken.h"
#include "object/object-flags.h"
#include "object/object-kind.h"
#include "object/object-value-calc.h"
#include "perception/object-perception.h"
#include "system/artifact-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 未鑑定なベースアイテムの基本価格を返す /
 * Return the "value" of an "unknown" item Make a guess at the value of non-aware items
 * @param o_ptr 未鑑定価格を確認したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトの未鑑定価格
 */
static PRICE object_value_base(const ObjectType *o_ptr)
{
    if (o_ptr->is_aware())
        return k_info[o_ptr->k_idx].cost;

    switch (o_ptr->tval) {
    case ItemKindType::FOOD:
        return 5;
    case ItemKindType::POTION:
        return 20;
    case ItemKindType::SCROLL:
        return 20;
    case ItemKindType::STAFF:
        return 70;
    case ItemKindType::WAND:
        return 50;
    case ItemKindType::ROD:
        return 90;
    case ItemKindType::RING:
        return 45;
    case ItemKindType::AMULET:
        return 45;
    case ItemKindType::FIGURINE: {
        DEPTH level = r_info[o_ptr->pval].level;
        if (level < 20)
            return level * 50L;
        else if (level < 30)
            return 1000 + (level - 20) * 150;
        else if (level < 40)
            return 2500 + (level - 30) * 350;
        else if (level < 50)
            return 6000 + (level - 40) * 800;
        else
            return 14000 + (level - 50) * 2000;
    }
    case ItemKindType::CAPTURE:
        if (!o_ptr->pval)
            return 1000;
        else
            return (r_info[o_ptr->pval].level) * 50 + 1000;

    default:
        break;
    }

    return 0;
}

/*!
 * @brief オブジェクト価格算出のメインルーチン /
 * Return the price of an item including plusses (and charges)
 * @param o_ptr 判明している現価格を確認したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトの判明している現価格
 * @details
 * This function returns the "value" of the given item (qty one)\n
 *\n
 * Never notice "unknown" bonuses or properties, including "curses",\n
 * since that would give the player information he did not have.\n
 *\n
 * Note that discounted items stay discounted forever, even if\n
 * the discount is "forgotten" by the player via memory loss.\n
 */
PRICE object_value(const ObjectType *o_ptr)
{
    PRICE value;

    if (o_ptr->is_known()) {
        if (o_ptr->is_broken())
            return 0;
        if (o_ptr->is_cursed())
            return 0;

        value = object_value_real(o_ptr);
    } else {
        if ((o_ptr->ident & (IDENT_SENSE)) && o_ptr->is_broken())
            return 0;
        if ((o_ptr->ident & (IDENT_SENSE)) && o_ptr->is_cursed())
            return 0;

        value = object_value_base(o_ptr);
    }

    if (o_ptr->discount)
        value -= (value * o_ptr->discount / 100L);

    return value;
}

/*!
 * @brief オブジェクトの真の価格を算出する /
 * Return the value of the flags the object has...
 * @param o_ptr 本価格を確認したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトの本価格
 * @details
 * Return the "real" price of a "known" item, not including discounts\n
 *\n
 * Wand and staffs get cost for each charge\n
 *\n
 * Armor is worth an extra 100 gold per bonus point to armor class.\n
 *\n
 * Weapons are worth an extra 100 gold per bonus point (AC,TH,TD).\n
 *\n
 * Missiles are only worth 5 gold per bonus point, since they\n
 * usually appear in groups of 20, and we want the player to get\n
 * the same amount of cash for any "equivalent" item.  Note that\n
 * missiles never have any of the "pval" flags, and in fact, they\n
 * only have a few of the available flags, primarily of the "slay"\n
 * and "brand" and "ignore" variety.\n
 *\n
 * Armor with a negative armor bonus is worthless.\n
 * Weapons with negative hit+damage bonuses are worthless.\n
 *\n
 * Every wearable item with a "pval" bonus is worth extra (see below).\n
 */
PRICE object_value_real(const ObjectType *o_ptr)
{
    auto *k_ptr = &k_info[o_ptr->k_idx];

    if (!k_info[o_ptr->k_idx].cost)
        return 0;

    PRICE value = k_info[o_ptr->k_idx].cost;
    auto flgs = object_flags(o_ptr);
    if (o_ptr->is_fixed_artifact()) {
        auto *a_ptr = &a_info[o_ptr->name1];
        if (!a_ptr->cost)
            return 0;

        value = a_ptr->cost;
        value += flag_cost(o_ptr, o_ptr->pval);
        return value;
    } else if (o_ptr->is_ego()) {
        auto *e_ptr = &e_info[o_ptr->name2];
        if (!e_ptr->cost)
            return 0;

        value += e_ptr->cost;
        value += flag_cost(o_ptr, o_ptr->pval);
    } else {
        if (o_ptr->art_flags.any())
            value += flag_cost(o_ptr, o_ptr->pval);
    }

    /* Analyze pval bonus for normal object */
    switch (o_ptr->tval) {
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
    case ItemKindType::BOW:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
    case ItemKindType::SHIELD:
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::LITE:
    case ItemKindType::AMULET:
    case ItemKindType::RING:
        if (!o_ptr->pval)
            break;
        if (o_ptr->pval < 0)
            return 0;

        if (flgs.has(TR_STR))
            value += (o_ptr->pval * 200L);
        if (flgs.has(TR_INT))
            value += (o_ptr->pval * 200L);
        if (flgs.has(TR_WIS))
            value += (o_ptr->pval * 200L);
        if (flgs.has(TR_DEX))
            value += (o_ptr->pval * 200L);
        if (flgs.has(TR_CON))
            value += (o_ptr->pval * 200L);
        if (flgs.has(TR_CHR))
            value += (o_ptr->pval * 200L);
        if (flgs.has(TR_MAGIC_MASTERY))
            value += (o_ptr->pval * 100);
        if (flgs.has(TR_STEALTH))
            value += (o_ptr->pval * 100L);
        if (flgs.has(TR_SEARCH))
            value += (o_ptr->pval * 100L);
        if (flgs.has(TR_INFRA))
            value += (o_ptr->pval * 50L);
        if (flgs.has(TR_TUNNEL))
            value += (o_ptr->pval * 50L);
        if (flgs.has(TR_BLOWS))
            value += (o_ptr->pval * 5000L);
        if (flgs.has(TR_SPEED))
            value += (o_ptr->pval * 10000L);
        break;

    default:
        break;
    }

    switch (o_ptr->tval) {
    case ItemKindType::WAND: {
        /* Pay extra for charges, depending on standard number of
         * charges.  Handle new-style wands correctly. -LM-
         */
        value += (value * o_ptr->pval / o_ptr->number / (k_ptr->pval * 2));
        break;
    }
    case ItemKindType::STAFF: {
        /* Pay extra for charges, depending on standard number of
         * charges.  -LM-
         */
        value += (value * o_ptr->pval / (k_ptr->pval * 2));
        break;
    }
    case ItemKindType::RING:
    case ItemKindType::AMULET: {
        if (o_ptr->to_h + o_ptr->to_d + o_ptr->to_a < 0)
            return 0;

        value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 200L);
        break;
    }
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::CLOAK:
    case ItemKindType::CROWN:
    case ItemKindType::HELM:
    case ItemKindType::SHIELD:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR: {
        if (o_ptr->to_a < 0)
            return 0;

        value += (((o_ptr->to_h - k_ptr->to_h) + (o_ptr->to_d - k_ptr->to_d)) * 200L + (o_ptr->to_a) * 100L);
        break;
    }
    case ItemKindType::BOW:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::SWORD:
    case ItemKindType::POLEARM: {
        if (o_ptr->to_h + o_ptr->to_d < 0)
            return 0;

        value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
        value += (o_ptr->dd - k_ptr->dd) * o_ptr->ds * 250L;
        value += (o_ptr->ds - k_ptr->ds) * o_ptr->dd * 250L;
        break;
    }
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT: {
        if (o_ptr->to_h + o_ptr->to_d < 0)
            return 0;

        value += ((o_ptr->to_h + o_ptr->to_d) * 5L);
        value += (o_ptr->dd - k_ptr->dd) * o_ptr->ds * 5L;
        value += (o_ptr->ds - k_ptr->ds) * o_ptr->dd * 5L;
        break;
    }
    case ItemKindType::FIGURINE: {
        DEPTH level = r_info[o_ptr->pval].level;
        if (level < 20)
            value = level * 50L;
        else if (level < 30)
            value = 1000 + (level - 20) * 150L;
        else if (level < 40)
            value = 2500 + (level - 30) * 350L;
        else if (level < 50)
            value = 6000 + (level - 40) * 800L;
        else
            value = 14000 + (level - 50) * 2000L;
        break;
    }
    case ItemKindType::CAPTURE: {
        if (!o_ptr->pval)
            value = 1000L;
        else
            value = ((r_info[o_ptr->pval].level) * 50L + 1000);
        break;
    }
    case ItemKindType::CHEST: {
        if (!o_ptr->pval)
            value = 0L;
        break;
    }

    default:
        break;
    }

    if (value < 0)
        return 0L;

    return value;
}
