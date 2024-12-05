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
#include "system/item-entity.h"
#include <algorithm>

Store *st_ptr = nullptr;

/*!
 * @brief 店舗のオブジェクト数を増やす
 * @param i_idx 増やしたいアイテムのインベントリID
 * @param num 増やしたい数
 */
void Store::increase_item(short i_idx, int item_num)
{
    auto &item = *this->stock[i_idx];
    auto cnt = item.number + item_num;
    if (cnt > 255) {
        cnt = 255;
    } else if (cnt < 0) {
        cnt = 0;
    }

    item_num = cnt - item.number;
    item.number += item_num;
}

/*!
 * @brief 店舗のオブジェクト数を削除する
 * @param i_idx 削除したいアイテムのID
 */
void store_item_optimize(short i_idx)
{
    const auto &item = *st_ptr->stock[i_idx];
    if (!item.is_valid() || (item.number != 0)) {
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

    st_ptr->increase_item(what, -num);
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
        auto &item = *st_ptr->stock[i];
        if ((&item == j_ptr) || (item.bi_id != j_ptr->bi_id) || !item.is_wand_staff()) {
            continue;
        }

        list.push_back(item.pval);
    }

    return list;
}

/*!
 * @brief 店舗に並べた品を重ね合わせる
 * @param item1 重ね合わせられて残る方のアイテムへの参照
 * @param item2 重ね合わせるて消える方のアイテムへの参照
 */
static void store_object_absorb(ItemEntity &item1, const ItemEntity &item2)
{
    const auto max_num = (item1.bi_key.tval() == ItemKindType::ROD) ? std::min(99, MAX_SHORT / item1.get_baseitem_pval()) : 99;
    const auto total = item1.number + item2.number;
    const auto diff = (total > max_num) ? total - max_num : 0;
    item1.number = (total > max_num) ? max_num : total;
    if (item1.is_wand_rod()) {
        item1.pval += item2.pval * (item2.number - diff) / item2.number;
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
        auto &item = *st_ptr->stock[slot];
        if (item.is_similar_for_store(*o_ptr)) {
            store_object_absorb(item, *o_ptr);
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
    *st_ptr->stock[slot] = o_ptr->clone();
    return slot;
}
