/*!
 * @brief 店舗処理関係のユーティリティ
 * @date 2020/03/20
 * @author Hourier
 */

#include "store/store-util.h"
#include "store/store-owners.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-kind.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/object-type-definition.h"
#include "world/world-object.h"

StoreSaleType cur_store_num = StoreSaleType::GENERAL;
store_type *st_ptr = nullptr;

/*!
 * @brief 店舗のオブジェクト数を増やす /
 * Add the item "o_ptr" to a real stores inventory.
 * @param item 増やしたいアイテムのID
 * @param num 増やしたい数
 * @details
 * <pre>
 * Increase, by a given amount, the number of a certain item
 * in a certain store.	This can result in zero items.
 * </pre>
 */
void store_item_increase(INVENTORY_IDX item, ITEM_NUMBER num)
{
    object_type *o_ptr;
    o_ptr = &st_ptr->stock[item];
    int cnt = o_ptr->number + num;
    if (cnt > 255)
        cnt = 255;
    else if (cnt < 0)
        cnt = 0;

    num = cnt - o_ptr->number;
    o_ptr->number += num;
}

/*!
 * @brief 店舗のオブジェクト数を削除する /
 * Remove a slot if it is empty
 * @param item 削除したいアイテムのID
 */
void store_item_optimize(INVENTORY_IDX item)
{
    object_type *o_ptr;
    o_ptr = &st_ptr->stock[item];
    if ((o_ptr->k_idx == 0) || (o_ptr->number != 0))
        return;

    st_ptr->stock_num--;
    for (int j = item; j < st_ptr->stock_num; j++)
        st_ptr->stock[j] = st_ptr->stock[j + 1];

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
    if (randint0(100) < 50)
        num = (num + 1) / 2;

    if (randint0(100) < 50)
        num = 1;

    if ((st_ptr->stock[what].tval == ItemKindType::ROD) || (st_ptr->stock[what].tval == ItemKindType::WAND))
        st_ptr->stock[what].pval -= num * st_ptr->stock[what].pval / st_ptr->stock[what].number;

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
static std::vector<PARAMETER_VALUE> store_same_magic_device_pvals(object_type *j_ptr)
{
    auto list = std::vector<PARAMETER_VALUE>();
    for (INVENTORY_IDX i = 0; i < st_ptr->stock_num; i++) {
        object_type *o_ptr = &st_ptr->stock[i];
        if (o_ptr == j_ptr)
            continue;
        if (o_ptr->k_idx != j_ptr->k_idx)
            continue;
        if (o_ptr->tval != ItemKindType::STAFF && o_ptr->tval != ItemKindType::WAND)
            continue;
        list.push_back(o_ptr->pval);
    }
    return list;
}

/*!
 * @brief 店舗の品揃え変化のためにアイテムを追加する /
 * Creates a random item and gives it to a store
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * <pre>
 * This algorithm needs to be rethought.  A lot.
 * Currently, "normal" stores use a pre-built array.
 * Note -- the "level" given to "obj_get_num()" is a "favored"
 * level, that is, there is a much higher chance of getting
 * items with a level approaching that of the given level...
 * Should we check for "permission" to have the given item?
 * </pre>
 */
void store_create(
    PlayerType *player_ptr, KIND_OBJECT_IDX fix_k_idx, black_market_crap_pf black_market_crap, store_will_buy_pf store_will_buy, mass_produce_pf mass_produce)
{
    if (st_ptr->stock_num >= st_ptr->stock_size)
        return;

    const owner_type *ow_ptr = &owners[enum2i(cur_store_num)][st_ptr->owner];

    for (int tries = 0; tries < 4; tries++) {
        KIND_OBJECT_IDX k_idx;
        DEPTH level;
        if (cur_store_num == StoreSaleType::BLACK) {
            level = 25 + randint0(25);
            k_idx = get_obj_num(player_ptr, level, 0x00000000);
            if (k_idx == 0)
                continue;
        } else if (fix_k_idx > 0) {
            k_idx = fix_k_idx;
            level = rand_range(1, ow_ptr->level);
        } else {
            k_idx = st_ptr->table[randint0(st_ptr->table.size())];
            level = rand_range(1, ow_ptr->level);
        }

        object_type forge;
        object_type *q_ptr;
        q_ptr = &forge;
        q_ptr->prep(k_idx);
        apply_magic_to_object(player_ptr, q_ptr, level, AM_NO_FIXED_ART);
        if (!(*store_will_buy)(player_ptr, q_ptr))
            continue;

        auto pvals = store_same_magic_device_pvals(q_ptr);
        if (pvals.size() >= 2) {
            auto pval = pvals.at(randint0(pvals.size()));
            q_ptr->pval = pval;
        }

        if (q_ptr->tval == ItemKindType::LITE) {
            if (q_ptr->sval == SV_LITE_TORCH)
                q_ptr->xtra4 = FUEL_TORCH / 2;

            if (q_ptr->sval == SV_LITE_LANTERN)
                q_ptr->xtra4 = FUEL_LAMP / 2;
        }

        object_known(q_ptr);
        q_ptr->ident |= IDENT_STORE;
        if (q_ptr->tval == ItemKindType::CHEST)
            continue;

        if (cur_store_num == StoreSaleType::BLACK) {
            if (black_market_crap(player_ptr, q_ptr) || (object_value(q_ptr) < 10))
                continue;
        } else {
            if (object_value(q_ptr) <= 0)
                continue;
        }

        mass_produce(player_ptr, q_ptr);
        (void)store_carry(q_ptr);
        break;
    }
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
bool store_object_similar(object_type *o_ptr, object_type *j_ptr)
{
    if (o_ptr == j_ptr)
        return false;

    if (o_ptr->k_idx != j_ptr->k_idx)
        return false;

    if ((o_ptr->pval != j_ptr->pval) && (o_ptr->tval != ItemKindType::WAND) && (o_ptr->tval != ItemKindType::ROD))
        return false;

    if (o_ptr->to_h != j_ptr->to_h)
        return false;

    if (o_ptr->to_d != j_ptr->to_d)
        return false;

    if (o_ptr->to_a != j_ptr->to_a)
        return false;

    if (o_ptr->name2 != j_ptr->name2)
        return false;

    if (o_ptr->is_artifact() || j_ptr->is_artifact())
        return false;

    if (o_ptr->art_flags != j_ptr->art_flags)
        return false;

    if (o_ptr->xtra1 || j_ptr->xtra1)
        return false;

    if (o_ptr->timeout || j_ptr->timeout)
        return false;

    if (o_ptr->ac != j_ptr->ac)
        return false;

    if (o_ptr->dd != j_ptr->dd)
        return false;

    if (o_ptr->ds != j_ptr->ds)
        return false;

    if (o_ptr->tval == ItemKindType::CHEST)
        return false;

    if (o_ptr->tval == ItemKindType::STATUE)
        return false;

    if (o_ptr->tval == ItemKindType::CAPTURE)
        return false;

    if (o_ptr->discount != j_ptr->discount)
        return false;

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
static void store_object_absorb(object_type *o_ptr, object_type *j_ptr)
{
    int max_num = (o_ptr->tval == ItemKindType::ROD) ? std::min(99, MAX_SHORT / k_info[o_ptr->k_idx].pval) : 99;
    int total = o_ptr->number + j_ptr->number;
    int diff = (total > max_num) ? total - max_num : 0;
    o_ptr->number = (total > max_num) ? max_num : total;
    if ((o_ptr->tval == ItemKindType::ROD) || (o_ptr->tval == ItemKindType::WAND))
        o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
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
int store_carry(object_type *o_ptr)
{
    PRICE value = object_value(o_ptr);
    if (value <= 0)
        return -1;

    o_ptr->ident |= IDENT_FULL_KNOWN;
    o_ptr->inscription = 0;
    o_ptr->feeling = FEEL_NONE;
    int slot;
    for (slot = 0; slot < st_ptr->stock_num; slot++) {
        object_type *j_ptr;
        j_ptr = &st_ptr->stock[slot];
        if (store_object_similar(j_ptr, o_ptr)) {
            store_object_absorb(j_ptr, o_ptr);
            return slot;
        }
    }

    if (st_ptr->stock_num >= st_ptr->stock_size)
        return -1;

    for (slot = 0; slot < st_ptr->stock_num; slot++) {
        object_type *j_ptr;
        j_ptr = &st_ptr->stock[slot];
        if (o_ptr->tval > j_ptr->tval)
            break;
        if (o_ptr->tval < j_ptr->tval)
            continue;
        if (o_ptr->sval < j_ptr->sval)
            break;
        if (o_ptr->sval > j_ptr->sval)
            continue;
        if (o_ptr->tval == ItemKindType::ROD) {
            if (o_ptr->pval < j_ptr->pval)
                break;
            if (o_ptr->pval > j_ptr->pval)
                continue;
        }

        PRICE j_value = object_value(j_ptr);
        if (value > j_value)
            break;
        if (value < j_value)
            continue;
    }

    for (int i = st_ptr->stock_num; i > slot; i--)
        st_ptr->stock[i] = st_ptr->stock[i - 1];

    st_ptr->stock_num++;
    st_ptr->stock[slot] = *o_ptr;
    return slot;
}
