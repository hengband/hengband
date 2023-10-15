/*!
 * @brief 店舗処理関係のユーティリティ
 * @date 2020/03/20
 * @author Hourier
 */

#include "store/store-util.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"

store_type *st_ptr = nullptr;

/*!
 * @brief 店舗のオブジェクト数を増やす
 * @param i_idx 増やしたいアイテムのインベントリID
 * @param num 増やしたい数
 */
void store_item_increase(INVENTORY_IDX i_idx, ITEM_NUMBER num)
{
    ItemEntity *o_ptr;
    o_ptr = &st_ptr->stock[i_idx];
    int cnt = o_ptr->number + num;
    if (cnt > 255) {
        cnt = 255;
    } else if (cnt < 0) {
        cnt = 0;
    }

    num = cnt - o_ptr->number;
    o_ptr->number += num;
}

/*!
 * @brief 店舗のオブジェクト数を削除する
 * @param i_idx 削除したいアイテムのID
 */
void store_item_optimize(INVENTORY_IDX i_idx)
{
    const auto *o_ptr = &st_ptr->stock[i_idx];
    if (!o_ptr->is_valid() || (o_ptr->number != 0)) {
        return;
    }

    st_ptr->stock_num--;
    for (int j = i_idx; j < st_ptr->stock_num; j++) {
        st_ptr->stock[j] = st_ptr->stock[j + 1];
    }

    (&st_ptr->stock[st_ptr->stock_num])->wipe();
}

/*!
 * @brief 店舗の品揃え変化のためにアイテムを削除する /
 * Attempt to delete (some of) a random item from the store
 * @details
 * <pre>
 * Hack -- we attempt to "maintain" piles of items when possible.
 * </pre>
 */
void store_delete(void)
{
    INVENTORY_IDX what = (INVENTORY_IDX)randint0(st_ptr->stock_num);
    int num = st_ptr->stock[what].number;
    if (randint0(100) < 50) {
        num = (num + 1) / 2;
    }

    if (randint0(100) < 50) {
        num = 1;
    }

    if (st_ptr->stock[what].is_wand_rod()) {
        st_ptr->stock[what].pval -= num * st_ptr->stock[what].pval / st_ptr->stock[what].number;
    }

    store_item_increase(what, -num);
    store_item_optimize(what);
}

/*!
 * @brief 店舗販売中の杖と魔法棒のpvalのリストを返す
 * @param j_ptr これから売ろうとしているオブジェクト
 * @return plavリスト(充填数)
 * @details
 * 回数の違う杖と魔法棒がスロットを圧迫するのでスロット数制限をかける
 */
std::vector<PARAMETER_VALUE> store_same_magic_device_pvals(ItemEntity *j_ptr)
{
    auto list = std::vector<PARAMETER_VALUE>();
    for (INVENTORY_IDX i = 0; i < st_ptr->stock_num; i++) {
        auto *o_ptr = &st_ptr->stock[i];
        if ((o_ptr == j_ptr) || (o_ptr->bi_id != j_ptr->bi_id) || !o_ptr->is_wand_staff()) {
            continue;
        }

        list.push_back(o_ptr->pval);
    }

    return list;
}

/*!
 * @brief 店舗に並べた品を同一品であるかどうか判定する /
 * Determine if a store item can "absorb" another item
 * @param o_ptr 判定するオブジェクト構造体の参照ポインタ1
 * @param j_ptr 判定するオブジェクト構造体の参照ポインタ2
 * @return 同一扱いできるならTRUEを返す
 * @details
 * <pre>
 * See "object_similar()" for the same function for the "player"
 * </pre>
 */
bool store_object_similar(ItemEntity *o_ptr, ItemEntity *j_ptr)
{
    if (o_ptr == j_ptr) {
        return false;
    }

    if (o_ptr->bi_id != j_ptr->bi_id) {
        return false;
    }

    if ((o_ptr->pval != j_ptr->pval) && !o_ptr->is_wand_rod()) {
        return false;
    }

    if (o_ptr->to_h != j_ptr->to_h) {
        return false;
    }

    if (o_ptr->to_d != j_ptr->to_d) {
        return false;
    }

    if (o_ptr->to_a != j_ptr->to_a) {
        return false;
    }

    if (o_ptr->ego_idx != j_ptr->ego_idx) {
        return false;
    }

    if (o_ptr->is_fixed_or_random_artifact() || j_ptr->is_fixed_or_random_artifact()) {
        return false;
    }

    if (o_ptr->art_flags != j_ptr->art_flags) {
        return false;
    }

    if (o_ptr->timeout || j_ptr->timeout) {
        return false;
    }

    if (o_ptr->ac != j_ptr->ac) {
        return false;
    }

    if (o_ptr->dd != j_ptr->dd) {
        return false;
    }

    if (o_ptr->ds != j_ptr->ds) {
        return false;
    }

    const auto tval = o_ptr->bi_key.tval();
    if (tval == ItemKindType::CHEST) {
        return false;
    }

    if (tval == ItemKindType::STATUE) {
        return false;
    }

    if (tval == ItemKindType::CAPTURE) {
        return false;
    }

    if ((tval == ItemKindType::LITE) && (o_ptr->fuel != j_ptr->fuel)) {
        return false;
    }

    if (o_ptr->discount != j_ptr->discount) {
        return false;
    }

    return true;
}

/*!
 * @brief 店舗に並べた品を重ね合わせできるかどうか判定する /
 * Allow a store item to absorb another item
 * @param o_ptr 判定するオブジェクト構造体の参照ポインタ1
 * @param j_ptr 判定するオブジェクト構造体の参照ポインタ2
 * @return 重ね合わせできるならTRUEを返す
 * @details
 * <pre>
 * See "object_similar()" for the same function for the "player"
 * </pre>
 */
static void store_object_absorb(ItemEntity *o_ptr, ItemEntity *j_ptr)
{
    int max_num = (o_ptr->bi_key.tval() == ItemKindType::ROD) ? std::min(99, MAX_SHORT / o_ptr->get_baseitem().pval) : 99;
    int total = o_ptr->number + j_ptr->number;
    int diff = (total > max_num) ? total - max_num : 0;
    o_ptr->number = (total > max_num) ? max_num : total;
    if (o_ptr->is_wand_rod()) {
        o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
    }
}

/*!
 * @brief 店舗にオブジェクトを加える /
 * Add the item "o_ptr" to a real stores inventory.
 * @param o_ptr 加えたいオブジェクトの構造体参照ポインタ
 * @return 収めた先のID
 * @details
 * <pre>
 * In all cases, return the slot (or -1) where the object was placed
 * Note that this is a hacked up version of "store_item_to_inventory()".
 * Also note that it may not correctly "adapt" to "knowledge" bacoming
 * known, the player may have to pick stuff up and drop it again.
 * </pre>
 */
int store_carry(ItemEntity *o_ptr)
{
    const auto value = o_ptr->get_price();
    if (value <= 0) {
        return -1;
    }

    o_ptr->ident |= IDENT_FULL_KNOWN;
    o_ptr->inscription.reset();
    o_ptr->feeling = FEEL_NONE;
    int slot;
    for (slot = 0; slot < st_ptr->stock_num; slot++) {
        ItemEntity *j_ptr;
        j_ptr = &st_ptr->stock[slot];
        if (store_object_similar(j_ptr, o_ptr)) {
            store_object_absorb(j_ptr, o_ptr);
            return slot;
        }
    }

    if (st_ptr->stock_num >= st_ptr->stock_size) {
        return -1;
    }

    const auto o_tval = o_ptr->bi_key.tval();
    const auto o_sval = o_ptr->bi_key.sval();
    for (slot = 0; slot < st_ptr->stock_num; slot++) {
        const auto *j_ptr = &st_ptr->stock[slot];
        const auto j_tval = j_ptr->bi_key.tval();
        const auto j_sval = j_ptr->bi_key.sval();
        if (o_tval > j_tval) {
            break;
        }

        if (o_tval < j_tval) {
            continue;
        }

        if (o_sval < j_sval) {
            break;
        }

        if (o_sval > j_sval) {
            continue;
        }

        if (o_tval == ItemKindType::ROD) {
            if (o_ptr->pval < j_ptr->pval) {
                break;
            }
            if (o_ptr->pval > j_ptr->pval) {
                continue;
            }
        }

        auto j_value = j_ptr->get_price();
        if (value > j_value) {
            break;
        }

        if (value < j_value) {
            continue;
        }
    }

    for (int i = st_ptr->stock_num; i > slot; i--) {
        st_ptr->stock[i] = st_ptr->stock[i - 1];
    }

    st_ptr->stock_num++;
    st_ptr->stock[slot] = *o_ptr;
    return slot;
}
