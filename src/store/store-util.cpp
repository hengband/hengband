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
#include <algorithm>

store_type *st_ptr = nullptr;

/*!
 * @brief 店舗のオブジェクト数を増やす
 * @param i_idx 増やしたいアイテムのインベントリID
 * @param num 増やしたい数
 */
void store_item_increase(short i_idx, int item_num)
{
    auto &item = st_ptr->stock[i_idx];
    auto cnt = item->number + item_num;
    if (cnt > 255) {
        cnt = 255;
    } else if (cnt < 0) {
        cnt = 0;
    }

    item_num = cnt - item->number;
    item->number += item_num;
}

/*!
 * @brief 店舗のオブジェクト数を削除する
 * @param i_idx 削除したいアイテムのID
 */
void store_item_optimize(short i_idx)
{
    const auto &item = st_ptr->stock[i_idx];
    if (!item->is_valid() || (item->number != 0)) {
        return;
    }

    const auto begin = st_ptr->stock.begin();
    std::rotate(begin + i_idx, begin + i_idx + 1, begin + st_ptr->stock_num);

    st_ptr->stock_num--;
    st_ptr->stock[st_ptr->stock_num]->wipe();
}

/*!
 * @brief 店舗の品揃え変化のためにアイテムを削除する
 */
void store_delete()
{
    const auto what = randnum0<short>(st_ptr->stock_num);
    int num = st_ptr->stock[what]->number;
    if (one_in_(2)) {
        num = (num + 1) / 2;
    }

    if (one_in_(2)) {
        num = 1;
    }

    if (st_ptr->stock[what]->is_wand_rod()) {
        st_ptr->stock[what]->pval -= num * st_ptr->stock[what]->pval / st_ptr->stock[what]->number;
    }

    store_item_increase(what, -num);
    store_item_optimize(what);
}

/*!
 * @brief 店舗販売中の杖と魔法棒のpvalのリストを返す
 * @param j_ptr これから売ろうとしているオブジェクト
 * @return plavリスト(充填数)
 * @details 回数の違う杖と魔法棒がスロットを圧迫するのでスロット数制限をかける
 */
std::vector<short> store_same_magic_device_pvals(ItemEntity *j_ptr)
{
    auto list = std::vector<short>();
    for (INVENTORY_IDX i = 0; i < st_ptr->stock_num; i++) {
        auto &item = st_ptr->stock[i];
        if ((item.get() == j_ptr) || (item->bi_id != j_ptr->bi_id) || !item->is_wand_staff()) {
            continue;
        }

        list.push_back(item->pval);
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
bool store_object_similar(const ItemEntity *o_ptr, const ItemEntity *j_ptr)
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

    if (o_ptr->damage_dice != j_ptr->damage_dice) {
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
 * @brief 店舗のアイテムをソートするための比較関数
 *
 * @param item1 比較対象アイテムへの参照
 * @param item2 比較対象アイテムへの参照
 * @return item1の方が上位ならばTRUEを返す。
 */
static bool store_item_sort_comp(const ItemEntity &item1, const ItemEntity &item2)
{
    const auto item1_tval = item1.bi_key.tval();
    const auto item2_tval = item2.bi_key.tval();
    if (item1_tval > item2_tval) {
        return true;
    }
    if (item1_tval < item2_tval) {
        return false;
    }

    const auto item1_sval = item1.bi_key.sval();
    const auto item2_sval = item2.bi_key.sval();
    if (item1_sval < item2_sval) {
        return true;
    }
    if (item1_sval > item2_sval) {
        return false;
    }

    if (item1_tval == ItemKindType::ROD) {
        if (item1.pval < item2.pval) {
            return true;
        }
        if (item1.pval > item2.pval) {
            return false;
        }
    }

    return item1.calc_price() > item2.calc_price();
}

/*!
 * @brief 店舗にオブジェクトを加える
 * @param o_ptr 加えたいオブジェクトの構造体参照ポインタ
 * @return 収めた先の商品インデックス、但し無価値アイテムは店頭に並べない.
 */
int store_carry(ItemEntity *o_ptr)
{
    const auto value = o_ptr->calc_price();
    if (value <= 0) {
        return -1;
    }

    o_ptr->ident |= IDENT_FULL_KNOWN;
    o_ptr->inscription.reset();
    o_ptr->feeling = FEEL_NONE;
    for (auto slot = 0; slot < st_ptr->stock_num; slot++) {
        auto &item = st_ptr->stock[slot];
        if (store_object_similar(item.get(), o_ptr)) {
            store_object_absorb(item.get(), o_ptr);
            return slot;
        }
    }

    if (st_ptr->stock_num >= st_ptr->stock_size) {
        return -1;
    }

    const auto first = st_ptr->stock.begin();
    const auto last = first + st_ptr->stock_num;
    const auto slot_it = std::find_if(first, last,
        [&](const auto &item) { return store_item_sort_comp(*o_ptr, *item); });
    const auto slot = std::distance(first, slot_it);

    std::rotate(first + slot, last, last + 1);

    st_ptr->stock_num++;
    *st_ptr->stock[slot] = *o_ptr;
    return slot;
}
