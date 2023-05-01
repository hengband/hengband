#include "object/object-value.h"
#include "monster-race/monster-race.h"
#include "object/object-flags.h"
#include "object/object-value-calc.h"
#include "object/tval-types.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"

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
PRICE object_value_real(const ItemEntity *o_ptr)
{
    const auto &baseitem = o_ptr->get_baseitem();

    if (!baseitem.cost) {
        return 0;
    }

    PRICE value = baseitem.cost;
    auto flags = object_flags(o_ptr);
    if (o_ptr->is_fixed_artifact()) {
        const auto &artifact = ArtifactsInfo::get_instance().get_artifact(o_ptr->fixed_artifact_idx);
        if (!artifact.cost) {
            return 0;
        }

        value = artifact.cost;
        value += flag_cost(o_ptr, o_ptr->pval);
        return value;
    } else if (o_ptr->is_ego()) {
        const auto &ego = o_ptr->get_ego();
        if (!ego.cost) {
            return 0;
        }

        value += ego.cost;
        value += flag_cost(o_ptr, o_ptr->pval);
    } else {
        if (o_ptr->art_flags.any()) {
            value += flag_cost(o_ptr, o_ptr->pval);
        }
    }

    /* Analyze pval bonus for normal object */
    switch (o_ptr->bi_key.tval()) {
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
        if (!o_ptr->pval) {
            break;
        }
        if (o_ptr->pval < 0) {
            return 0;
        }

        if (flags.has(TR_STR)) {
            value += (o_ptr->pval * 200L);
        }
        if (flags.has(TR_INT)) {
            value += (o_ptr->pval * 200L);
        }
        if (flags.has(TR_WIS)) {
            value += (o_ptr->pval * 200L);
        }
        if (flags.has(TR_DEX)) {
            value += (o_ptr->pval * 200L);
        }
        if (flags.has(TR_CON)) {
            value += (o_ptr->pval * 200L);
        }
        if (flags.has(TR_CHR)) {
            value += (o_ptr->pval * 200L);
        }
        if (flags.has(TR_MAGIC_MASTERY)) {
            value += (o_ptr->pval * 100);
        }
        if (flags.has(TR_STEALTH)) {
            value += (o_ptr->pval * 100L);
        }
        if (flags.has(TR_SEARCH)) {
            value += (o_ptr->pval * 100L);
        }
        if (flags.has(TR_INFRA)) {
            value += (o_ptr->pval * 50L);
        }
        if (flags.has(TR_TUNNEL)) {
            value += (o_ptr->pval * 50L);
        }
        if (flags.has(TR_BLOWS)) {
            value += (o_ptr->pval * 5000L);
        }
        if (flags.has(TR_SPEED)) {
            value += (o_ptr->pval * 10000L);
        }
        break;

    default:
        break;
    }

    switch (o_ptr->bi_key.tval()) {
    case ItemKindType::WAND: {
        /* Pay extra for charges, depending on standard number of
         * charges.  Handle new-style wands correctly. -LM-
         */
        value += (value * o_ptr->pval / o_ptr->number / (baseitem.pval * 2));
        break;
    }
    case ItemKindType::STAFF: {
        /* Pay extra for charges, depending on standard number of
         * charges.  -LM-
         */
        value += (value * o_ptr->pval / (baseitem.pval * 2));
        break;
    }
    case ItemKindType::RING:
    case ItemKindType::AMULET: {
        if (o_ptr->to_h + o_ptr->to_d + o_ptr->to_a < 0) {
            return 0;
        }

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
        if (o_ptr->to_a < 0) {
            return 0;
        }

        value += (((o_ptr->to_h - baseitem.to_h) + (o_ptr->to_d - baseitem.to_d)) * 200L + (o_ptr->to_a) * 100L);
        break;
    }
    case ItemKindType::BOW:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::SWORD:
    case ItemKindType::POLEARM: {
        if (o_ptr->to_h + o_ptr->to_d < 0) {
            return 0;
        }

        value += ((o_ptr->to_h + o_ptr->to_d + o_ptr->to_a) * 100L);
        value += (o_ptr->dd - baseitem.dd) * o_ptr->ds * 250L;
        value += (o_ptr->ds - baseitem.ds) * o_ptr->dd * 250L;
        break;
    }
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT: {
        if (o_ptr->to_h + o_ptr->to_d < 0) {
            return 0;
        }

        value += ((o_ptr->to_h + o_ptr->to_d) * 5L);
        value += (o_ptr->dd - baseitem.dd) * o_ptr->ds * 5L;
        value += (o_ptr->ds - baseitem.ds) * o_ptr->dd * 5L;
        break;
    }
    case ItemKindType::FIGURINE: {
        auto figure_r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
        DEPTH level = monraces_info[figure_r_idx].level;
        if (level < 20) {
            value = level * 50L;
        } else if (level < 30) {
            value = 1000 + (level - 20) * 150L;
        } else if (level < 40) {
            value = 2500 + (level - 30) * 350L;
        } else if (level < 50) {
            value = 6000 + (level - 40) * 800L;
        } else {
            value = 14000 + (level - 50) * 2000L;
        }
        break;
    }
    case ItemKindType::CAPTURE: {
        auto capture_r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
        if (!MonsterRace(capture_r_idx).is_valid()) {
            value = 1000L;
        } else {
            value = ((monraces_info[capture_r_idx].level) * 50L + 1000);
        }
        break;
    }
    case ItemKindType::CHEST: {
        if (!o_ptr->pval) {
            value = 0L;
        }
        break;
    }

    default:
        break;
    }

    if (value < 0) {
        return 0L;
    }

    return value;
}
