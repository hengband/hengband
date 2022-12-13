﻿/*!
 * @brief 同種のアイテムをインベントリや床に重ね合わせたり、その判断を行う処理
 * @date 2020/06/03
 * @author Hourier
 */

#include "object/object-stack.h"
#include "game-option/game-play-options.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "smith/object-smith.h"
#include "sv-definition/sv-other-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"

/*!
 * @brief 魔法棒やロッドのスロット分割時に使用回数を分配する /
 * Distribute charges of rods or wands.
 * @param o_ptr 分割元オブジェクトの構造体参照ポインタ source item
 * @param q_ptr 分割先オブジェクトの構造体参照ポインタ target item, must be of the same type as o_ptr
 * @param amt 分割したい回数量 number of items that are transfered
 * @details
 * Hack -- If rods or wands are dropped, the total maximum timeout or\n
 * charges need to be allocated between the two stacks.  If all the items\n
 * are being dropped, it makes for a neater message to leave the original\n
 * stack's pval alone. -LM-\n
 */
void distribute_charges(ItemEntity *o_ptr, ItemEntity *q_ptr, int amt)
{
    if (!o_ptr->is_wand_rod()) {
        return;
    }

    q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
    if (amt < o_ptr->number) {
        o_ptr->pval -= q_ptr->pval;
    }

    if ((o_ptr->bi_key.tval() != ItemKindType::ROD) || !o_ptr->timeout) {
        return;
    }

    if (q_ptr->pval > o_ptr->timeout) {
        q_ptr->timeout = o_ptr->timeout;
    } else {
        q_ptr->timeout = q_ptr->pval;
    }

    if (amt < o_ptr->number) {
        o_ptr->timeout -= q_ptr->timeout;
    }
}

/*!
 * @brief 魔法棒やロッドの使用回数を減らす /
 * @param o_ptr オブジェクトの構造体参照ポインタ source item
 * @param amt 減らしたい回数量 number of items that are transfered
 * @details
 * Hack -- If rods or wand are destroyed, the total maximum timeout or\n
 * charges of the stack needs to be reduced, unless all the items are\n
 * being destroyed. -LM-\n
 */
void reduce_charges(ItemEntity *o_ptr, int amt)
{
    if (o_ptr->is_wand_rod() && (amt < o_ptr->number)) {
        o_ptr->pval -= o_ptr->pval * amt / o_ptr->number;
    }
}

/*!
 * @brief 両オブジェクトをスロットに重ね合わせ可能な最大数を返す。
 * Determine if an item can partly absorb a second item. Return maximum number of stack.
 * @param o_ptr 検証したいオブジェクトの構造体参照ポインタ1
 * @param j_ptr 検証したいオブジェクトの構造体参照ポインタ2
 * @return 重ね合わせ可能なアイテム数
 */
int object_similar_part(const ItemEntity *o_ptr, const ItemEntity *j_ptr)
{
    const int max_stack_size = 99;
    int max_num = max_stack_size;
    if (o_ptr->bi_id != j_ptr->bi_id) {
        return 0;
    }

    switch (o_ptr->bi_key.tval()) {
    case ItemKindType::CHEST:
    case ItemKindType::CARD:
    case ItemKindType::CAPTURE: {
        return 0;
    }
    case ItemKindType::STATUE: {
        const auto o_sval = o_ptr->bi_key.sval();
        const auto j_sval = j_ptr->bi_key.sval();
        if ((o_sval != SV_PHOTO) || (j_sval != SV_PHOTO) || (o_ptr->pval != j_ptr->pval)) {
            return 0;
        }

        break;
    }
    case ItemKindType::FIGURINE:
    case ItemKindType::CORPSE: {
        if (o_ptr->pval != j_ptr->pval) {
            return 0;
        }

        break;
    }
    case ItemKindType::FOOD:
    case ItemKindType::POTION:
    case ItemKindType::SCROLL: {
        break;
    }
    case ItemKindType::STAFF: {
        if ((!(o_ptr->ident & (IDENT_EMPTY)) && !o_ptr->is_known()) || (!(j_ptr->ident & (IDENT_EMPTY)) && !j_ptr->is_known())) {
            return 0;
        }

        if (o_ptr->pval != j_ptr->pval) {
            return 0;
        }

        break;
    }
    case ItemKindType::WAND: {
        if ((!(o_ptr->ident & (IDENT_EMPTY)) && !o_ptr->is_known()) || (!(j_ptr->ident & (IDENT_EMPTY)) && !j_ptr->is_known())) {
            return 0;
        }

        break;
    }
    case ItemKindType::ROD: {
        max_num = std::min(max_num, MAX_SHORT / baseitems_info.at(o_ptr->bi_id).pval);
        break;
    }
    case ItemKindType::GLOVES:
        if (o_ptr->is_glove_same_temper(j_ptr)) {
            return 0;
        }

        if (!o_ptr->is_known() || !j_ptr->is_known()) {
            return 0;
        }

        if (!o_ptr->can_pile(j_ptr)) {
            return 0;
        }

        break;
    case ItemKindType::LITE:
        if (o_ptr->fuel != j_ptr->fuel) {
            return 0;
        }

        if (!o_ptr->can_pile(j_ptr)) {
            return 0;
        }

        break;
    case ItemKindType::BOW:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::BOOTS:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
    case ItemKindType::SHIELD:
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::RING:
    case ItemKindType::AMULET:
    case ItemKindType::WHISTLE:
        if (!o_ptr->is_known() || !j_ptr->is_known()) {
            return 0;
        }

        if (!o_ptr->can_pile(j_ptr)) {
            return 0;
        }

        break;
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::SHOT:
        if (!o_ptr->can_pile(j_ptr)) {
            return 0;
        }

        break;
    default: {
        if (!o_ptr->is_known() || !j_ptr->is_known()) {
            return 0;
        }

        break;
    }
    }

    if (o_ptr->art_flags != j_ptr->art_flags) {
        return 0;
    }

    if (o_ptr->curse_flags != j_ptr->curse_flags) {
        return 0;
    }
    if ((o_ptr->ident & (IDENT_BROKEN)) != (j_ptr->ident & (IDENT_BROKEN))) {
        return 0;
    }

    if (o_ptr->inscription && j_ptr->inscription && (o_ptr->inscription != j_ptr->inscription)) {
        return 0;
    }

    if (!stack_force_notes && (o_ptr->inscription != j_ptr->inscription)) {
        return 0;
    }
    if (!stack_force_costs && (o_ptr->discount != j_ptr->discount)) {
        return 0;
    }

    return max_num;
}

/*!
 * @brief 両オブジェクトをスロットに重ねることができるかどうかを返す。
 * Determine if an item can absorb a second item.
 * @param o_ptr 検証したいオブジェクトの構造体参照ポインタ1
 * @param j_ptr 検証したいオブジェクトの構造体参照ポインタ2
 * @return 重ね合わせ可能ならばTRUEを返す。
 */
bool object_similar(const ItemEntity *o_ptr, const ItemEntity *j_ptr)
{
    int total = o_ptr->number + j_ptr->number;
    int max_num = object_similar_part(o_ptr, j_ptr);
    if (!max_num) {
        return false;
    }
    if (total > max_num) {
        return 0;
    }

    return true;
}

/*!
 * @brief 両オブジェクトをスロットに重ね合わせる。
 * Allow one item to "absorb" another, assuming they are similar
 * @param o_ptr 重ね合わせ先のオブジェクトの構造体参照ポインタ
 * @param j_ptr 重ね合わせ元のオブジェクトの構造体参照ポインタ
 */
void object_absorb(ItemEntity *o_ptr, ItemEntity *j_ptr)
{
    int max_num = object_similar_part(o_ptr, j_ptr);
    int total = o_ptr->number + j_ptr->number;
    int diff = (total > max_num) ? total - max_num : 0;

    o_ptr->number = (total > max_num) ? max_num : total;
    if (j_ptr->is_known()) {
        object_known(o_ptr);
    }

    if (((o_ptr->ident & IDENT_STORE) || (j_ptr->ident & IDENT_STORE)) && (!((o_ptr->ident & IDENT_STORE) && (j_ptr->ident & IDENT_STORE)))) {
        if (j_ptr->ident & IDENT_STORE) {
            j_ptr->ident &= 0xEF;
        }
        if (o_ptr->ident & IDENT_STORE) {
            o_ptr->ident &= 0xEF;
        }
    }

    if (j_ptr->is_fully_known()) {
        o_ptr->ident |= (IDENT_FULL_KNOWN);
    }
    if (j_ptr->inscription) {
        o_ptr->inscription = j_ptr->inscription;
    }
    if (j_ptr->feeling) {
        o_ptr->feeling = j_ptr->feeling;
    }
    if (o_ptr->discount < j_ptr->discount) {
        o_ptr->discount = j_ptr->discount;
    }

    const auto tval = o_ptr->bi_key.tval();
    if (tval == ItemKindType::ROD) {
        o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
        o_ptr->timeout += j_ptr->timeout * (j_ptr->number - diff) / j_ptr->number;
    }

    if (tval == ItemKindType::WAND) {
        o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
    }
}
