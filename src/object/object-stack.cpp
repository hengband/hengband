/*!
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
#include "smith/object-smith.h"
#include "sv-definition/sv-other-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "util/bit-flags-calculator.h"

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
