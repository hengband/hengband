/*!
 * @brief 店舗処理関係のユーティリティ
 * @date 2020/03/20
 * @author Hourier
 */

#include "store/store-util.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-enchant.h"
#include "object/object-generator.h"
#include "object/object-kind.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "world/world-object.h"

int cur_store_num = 0;
store_type *st_ptr = NULL;

/*!
 * @brief 店舗のオブジェクト数を増やす /
 * Add the item "o_ptr" to a real stores inventory.
 * @param item 増やしたいアイテムのID
 * @param num 増やしたい数
 * @return なし
 * @details
 * <pre>
 * Increase, by a given amount, the number of a certain item
 * in a certain store.	This can result in zero items.
 * </pre>
 * @todo numは本来ITEM_NUMBER型にしたい。
 */
void store_item_increase(INVENTORY_IDX item, int num)
{
    object_type *o_ptr;
    o_ptr = &st_ptr->stock[item];
    int cnt = o_ptr->number + num;
    if (cnt > 255)
        cnt = 255;
    else if (cnt < 0)
        cnt = 0;

    num = cnt - o_ptr->number;
    o_ptr->number += (ITEM_NUMBER)num;
}

/*!
 * @brief 店舗のオブジェクト数を削除する /
 * Remove a slot if it is empty
 * @param item 削除したいアイテムのID
 * @return なし
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

    object_wipe(&st_ptr->stock[st_ptr->stock_num]);
}

/*!
 * @brief 店舗の品揃え変化のためにアイテムを削除する /
 * Attempt to delete (some of) a random item from the store
 * @return なし
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

    if ((st_ptr->stock[what].tval == TV_ROD) || (st_ptr->stock[what].tval == TV_WAND))
        st_ptr->stock[what].pval -= num * st_ptr->stock[what].pval / st_ptr->stock[what].number;
    
    store_item_increase(what, -num);
    store_item_optimize(what);
}

/*!
 * @brief 店舗の品揃え変化のためにアイテムを追加する /
 * Creates a random item and gives it to a store
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
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
void store_create(player_type *player_ptr, black_market_crap_pf black_market_crap, store_will_buy_pf store_will_buy, mass_produce_pf mass_produce)
{
    if (st_ptr->stock_num >= st_ptr->stock_size)
        return;

    for (int tries = 0; tries < 4; tries++) {
        OBJECT_IDX i;
        DEPTH level;
        if (cur_store_num == STORE_BLACK) {
            level = 25 + randint0(25);
            i = get_obj_num(player_ptr, level, 0x00000000);
            if (i == 0)
                continue;
        } else {
            i = st_ptr->table[randint0(st_ptr->table_num)];
            level = rand_range(1, STORE_OBJ_LEVEL);
        }

        object_type forge;
        object_type *q_ptr;
        q_ptr = &forge;
        object_prep(player_ptr, q_ptr, i);
        apply_magic(player_ptr, q_ptr, level, AM_NO_FIXED_ART);
        if (!(*store_will_buy)(player_ptr, q_ptr))
            continue;

        if (q_ptr->tval == TV_LITE) {
            if (q_ptr->sval == SV_LITE_TORCH)
                q_ptr->xtra4 = FUEL_TORCH / 2;

            if (q_ptr->sval == SV_LITE_LANTERN)
                q_ptr->xtra4 = FUEL_LAMP / 2;
        }

        object_known(q_ptr);
        q_ptr->ident |= IDENT_STORE;
        if (q_ptr->tval == TV_CHEST)
            continue;

        if (cur_store_num == STORE_BLACK) {
            if (black_market_crap(player_ptr, q_ptr) || (object_value(player_ptr, q_ptr) < 10))
                continue;
        } else {
            if (object_value(player_ptr, q_ptr) <= 0)
                continue;
        }

        mass_produce(player_ptr, q_ptr);
        (void)store_carry(player_ptr, q_ptr);
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
        return FALSE;

    if (o_ptr->k_idx != j_ptr->k_idx)
        return FALSE;

    if ((o_ptr->pval != j_ptr->pval) && (o_ptr->tval != TV_WAND) && (o_ptr->tval != TV_ROD))
        return FALSE;

    if (o_ptr->to_h != j_ptr->to_h)
        return FALSE;

    if (o_ptr->to_d != j_ptr->to_d)
        return FALSE;

    if (o_ptr->to_a != j_ptr->to_a)
        return FALSE;

    if (o_ptr->name2 != j_ptr->name2)
        return FALSE;

    if (object_is_artifact(o_ptr) || object_is_artifact(j_ptr))
        return FALSE;

    for (int i = 0; i < TR_FLAG_SIZE; i++)
        if (o_ptr->art_flags[i] != j_ptr->art_flags[i])
            return FALSE;

    if (o_ptr->xtra1 || j_ptr->xtra1)
        return FALSE;

    if (o_ptr->timeout || j_ptr->timeout)
        return FALSE;

    if (o_ptr->ac != j_ptr->ac)
        return FALSE;

    if (o_ptr->dd != j_ptr->dd)
        return FALSE;

    if (o_ptr->ds != j_ptr->ds)
        return FALSE;

    if (o_ptr->tval == TV_CHEST)
        return FALSE;

    if (o_ptr->tval == TV_STATUE)
        return FALSE;

    if (o_ptr->tval == TV_CAPTURE)
        return FALSE;

    if (o_ptr->discount != j_ptr->discount)
        return FALSE;

    return TRUE;
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
    int max_num = (o_ptr->tval == TV_ROD) ? MIN(99, MAX_SHORT / k_info[o_ptr->k_idx].pval) : 99;
    int total = o_ptr->number + j_ptr->number;
    int diff = (total > max_num) ? total - max_num : 0;
    o_ptr->number = (total > max_num) ? max_num : total;
    if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
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
int store_carry(player_type *player_ptr, object_type *o_ptr)
{
    PRICE value = object_value(player_ptr, o_ptr);
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
        if (o_ptr->tval == TV_ROD) {
            if (o_ptr->pval < j_ptr->pval)
                break;
            if (o_ptr->pval > j_ptr->pval)
                continue;
        }

        PRICE j_value = object_value(player_ptr, j_ptr);
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
