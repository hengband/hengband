#include "load/store-loader.h"
#include "object/object-generator.h"
#include "floor/floor-town.h"
#include "load/angband-version-comparer.h"
#include "load/item-loader.h"
#include "load/load-util.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "player-info/avatar.h"
#include "store/store.h"
#include "util/object-sort.h"

/*!
 * @brief 店置きのアイテムオブジェクトを読み込む / Add the item "o_ptr" to the inventory of the "Home"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param store_ptr 店舗の参照ポインタ
 * @param o_ptr アイテムオブジェクト参照ポインタ
 * @return なし
 * @details
 * In all cases, return the slot (or -1) where the object was placed
 *
 * Note that this is a hacked up version of "store_item_to_inventory()".
 *
 * Also note that it may not correctly "adapt" to "knowledge" bacoming
 * known, the player may have to pick stuff up and drop it again.
 */
static void home_carry_load(player_type *player_ptr, store_type *store_ptr, object_type *o_ptr)
{
    for (int i = 0; i < store_ptr->stock_num; i++) {
        object_type *j_ptr;
        j_ptr = &store_ptr->stock[i];
        if (!object_similar(j_ptr, o_ptr))
            continue;

        object_absorb(j_ptr, o_ptr);
        return;
    }

    if (store_ptr->stock_num >= STORE_INVEN_MAX * 10)
        return;

    s32b value = object_value(player_ptr, o_ptr);
    int slot;
    for (slot = 0; slot < store_ptr->stock_num; slot++) {
        if (object_sort_comp(player_ptr, o_ptr, value, &store_ptr->stock[slot]))
            break;
    }

    for (int i = store_ptr->stock_num; i > slot; i--) {
        store_ptr->stock[i] = store_ptr->stock[i - 1];
    }

    store_ptr->stock_num++;
    store_ptr->stock[slot] = *o_ptr;
    chg_virtue(player_ptr, V_SACRIFICE, -1);
}

/*!
 * @brief 店舗情報を読み込む / Read a store
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param town_number 街ID
 * @param store_number 店舗ID
 * @return エラーID
 */
static errr rd_store(player_type *player_ptr, int town_number, int store_number)
{
    store_type *store_ptr;
    bool sort = FALSE;
    if (z_older_than(10, 3, 3) && (store_number == STORE_HOME)) {
        store_ptr = &town_info[1].store[store_number];
        if (store_ptr->stock_num)
            sort = TRUE;
    } else {
        store_ptr = &town_info[town_number].store[store_number];
    }

    byte own;
    byte tmp8u;
    s16b num;
    rd_s32b(&store_ptr->store_open);
    rd_s16b(&store_ptr->insult_cur);
    rd_byte(&own);
    if (z_older_than(11, 0, 4)) {
        rd_byte(&tmp8u);
        num = tmp8u;
    } else {
        rd_s16b(&num);
    }

    rd_s16b(&store_ptr->good_buy);
    rd_s16b(&store_ptr->bad_buy);

    rd_s32b(&store_ptr->last_visit);
    store_ptr->owner = own;

    for (int j = 0; j < num; j++) {
        object_type forge;
        object_type *q_ptr;
        q_ptr = &forge;
        object_wipe(q_ptr);

        rd_item(player_ptr, q_ptr);

        bool is_valid_item = store_ptr->stock_num
            < (store_number == STORE_HOME ? STORE_INVEN_MAX * 10 : store_number == STORE_MUSEUM ? STORE_INVEN_MAX * 50 : STORE_INVEN_MAX);
        if (!is_valid_item)
            continue;

        if (sort) {
            home_carry_load(player_ptr, store_ptr, q_ptr);
        } else {
            int k = store_ptr->stock_num++;
            object_copy(&store_ptr->stock[k], q_ptr);
        }
    }

    return 0;
}

errr load_store(player_type *creature_ptr)
{
    u16b tmp16u;
    rd_u16b(&tmp16u);
    int town_count = tmp16u;
    rd_u16b(&tmp16u);
    for (int i = 1; i < town_count; i++)
        for (int j = 0; j < tmp16u; j++)
            if (rd_store(creature_ptr, i, j))
                return 22;

    return 0;
}
