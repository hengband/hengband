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
static PRICE object_value_base(const object_type *o_ptr)
{
    if (o_ptr->is_aware())
        return (k_info[o_ptr->k_idx].cost);

    switch (o_ptr->tval) {
    case TV_FOOD:
        return (5L);
    case TV_POTION:
        return (20L);
    case TV_SCROLL:
        return (20L);
    case TV_STAFF:
        return (70L);
    case TV_WAND:
        return (50L);
    case TV_ROD:
        return (90L);
    case TV_RING:
        return (45L);
    case TV_AMULET:
        return (45L);
    case TV_FIGURINE: {
        DEPTH level = r_info[o_ptr->pval].level;
        if (level < 20)
            return level * 50L;
        else if (level < 30)
            return 1000 + (level - 20) * 150L;
        else if (level < 40)
            return 2500 + (level - 30) * 350L;
        else if (level < 50)
            return 6000 + (level - 40) * 800L;
        else
            return 14000 + (level - 50) * 2000L;
    }
    case TV_CAPTURE:
        if (!o_ptr->pval)
            return 1000L;
        else
            return ((r_info[o_ptr->pval].level) * 50L + 1000);

    default:
        break;
    }

    return (0L);
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
PRICE object_value(const object_type *o_ptr)
{
    PRICE value;

    if (o_ptr->is_known()) {
        if (o_ptr->is_broken())
            return (0L);
        if (o_ptr->is_cursed())
            return (0L);

        value = object_value_real(o_ptr);
    } else {
        if ((o_ptr->ident & (IDENT_SENSE)) && o_ptr->is_broken())
            return (0L);
        if ((o_ptr->ident & (IDENT_SENSE)) && o_ptr->is_cursed())
            return (0L);

        value = object_value_base(o_ptr);
    }

    if (o_ptr->discount)
        value -= (value * o_ptr->discount / 100L);

    return (value);
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
PRICE object_value_real(const object_type *o_ptr)
{
    object_kind *k_ptr = &k_info[o_ptr->k_idx];

    if (!k_info[o_ptr->k_idx].cost)
        return (0L);

    PRICE value = k_info[o_ptr->k_idx].cost;
    auto flgs = object_flags(o_ptr);
    if (o_ptr->is_fixed_artifact()) {
        artifact_type *a_ptr = &a_info[o_ptr->name1];
        if (!a_ptr->cost)
            return (0L);

        value = a_ptr->cost;
        value += flag_cost(o_ptr, o_ptr->pval);
        return (value);
    } else if (o_ptr->is_ego()) {
        ego_item_type *e_ptr = &e_info[o_ptr->name2];
        if (!e_ptr->cost)
            return (0L);

        value += e_ptr->cost;
        value += flag_cost(o_ptr, o_ptr->pval);
    } else {
        if (o_ptr->art_flags.any())
            value += flag_cost(o_ptr, o_ptr->pval);
    }

    /* Analyze pval bonus for normal object */
    switch (o_ptr->tval) {
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
    case TV_BOW:
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_HELM:
    case TV_CROWN:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
    case TV_LITE:
    case TV_AMULET:
    case TV_RING:
        if (!o_ptr->pval)
            break;
        if (o_ptr->pval < 0)
            return (0L);

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
    case TV_WAND: {
        /* Pay extra for charges, depending on standard number of
         * charges.  Handle new-style wands correctly. -LM-
         */
        value += (value * o_ptr->pval / o_ptr->number / (k_ptr->pval * 2));
        break;
    }
    case TV_STAFF: {
        /* Pay extra for charges, depending on standard number of
         * charges.  -LM-
         */
        value += (value * o_ptr->pval / (k_ptr->pval * 2));
        break;
    }
    case TV_RING:
    case TV_AMULET: {
        if (o_ptr->to_h + o_ptr->to_d + o_ptr->to_a < 0)
            return (0L);

        value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 200L);
        break;
    }
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_CLOAK:
    case TV_CROWN:
    case TV_HELM:
    case TV_SHIELD:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR: {
        if (o_ptr->to_a < 0)
            return (0L);

        value += (((o_ptr->to_h - k_ptr->to_h) + (o_ptr->to_d - k_ptr->to_d)) * 200L + (o_ptr->to_a) * 100L);
        break;
    }
    case TV_BOW:
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_SWORD:
    case TV_POLEARM: {
        if (o_ptr->to_h + o_ptr->to_d < 0)
            return (0L);

        value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
        value += (o_ptr->dd - k_ptr->dd) * o_ptr->ds * 250L;
        value += (o_ptr->ds - k_ptr->ds) * o_ptr->dd * 250L;
        break;
    }
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT: {
        if (o_ptr->to_h + o_ptr->to_d < 0)
            return (0L);

        value += ((o_ptr->to_h + o_ptr->to_d) * 5L);
        value += (o_ptr->dd - k_ptr->dd) * o_ptr->ds * 5L;
        value += (o_ptr->ds - k_ptr->ds) * o_ptr->dd * 5L;
        break;
    }
    case TV_FIGURINE: {
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
    case TV_CAPTURE: {
        if (!o_ptr->pval)
            value = 1000L;
        else
            value = ((r_info[o_ptr->pval].level) * 50L + 1000);
        break;
    }
    case TV_CHEST: {
        if (!o_ptr->pval)
            value = 0L;
        break;
    }

    default:
        break;
    }

    if (value < 0)
        return 0L;

    return (value);
}
