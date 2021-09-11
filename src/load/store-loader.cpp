#include "load/store-loader.h"
#include "avatar/avatar.h"
#include "floor/floor-town.h"
#include "load/angband-version-comparer.h"
#include "load/item-loader.h"
#include "load/load-util.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "store/store.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/object-sort.h"

/*!
 * @brief 店置きのアイテムオブジェクトを読み込む / Add the item "o_ptr" to the inventory of the "Home"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param store_ptr 店舗の参照ポインタ
 * @param o_ptr アイテムオブジェクト参照ポインタ
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

    if (store_ptr->stock_num >= store_get_stock_max(STORE_HOME))
        return;

    int32_t value = object_value(o_ptr);
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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param town_number 街ID
 * @param store_number 店舗ID
 * @return エラーID
 */
static errr rd_store(player_type *player_ptr, int town_number, int store_number)
{
    store_type *store_ptr;
    bool sort = false;
    if (h_older_than(0, 3, 3) && (store_number == STORE_HOME)) {
        store_ptr = &town_info[1].store[store_number];
        if (store_ptr->stock_num)
            sort = true;
    } else {
        store_ptr = &town_info[town_number].store[store_number];
    }

    byte owner_idx;
    byte tmp8u;
    int16_t inven_num;
    rd_s32b(&store_ptr->store_open);
    rd_s16b(&store_ptr->insult_cur);
    rd_byte(&owner_idx);
    if (h_older_than(1, 0, 4)) {
        rd_byte(&tmp8u);
        inven_num = tmp8u;
    } else {
        rd_s16b(&inven_num);
    }

    rd_s16b(&store_ptr->good_buy);
    rd_s16b(&store_ptr->bad_buy);

    rd_s32b(&store_ptr->last_visit);
    store_ptr->owner = owner_idx;

    for (int j = 0; j < inven_num; j++) {
        object_type forge;
        object_type *q_ptr;
        q_ptr = &forge;
        q_ptr->wipe();

        rd_item(q_ptr);

        auto stock_max = store_get_stock_max(static_cast<STORE_TYPE_IDX>(store_number));
        if (store_ptr->stock_num >= stock_max)
            continue;

        if (sort) {
            home_carry_load(player_ptr, store_ptr, q_ptr);
        } else {
            int k = store_ptr->stock_num++;
            (&store_ptr->stock[k])->copy_from(q_ptr);
        }
    }

    return 0;
}

/*!
 * @brief 店舗情報を読み込む
 * @param player_ptr プレイヤー情報への参照ポインタ(未使用)
 * @return 読み込み終わったら0、失敗したら22
 */
errr load_store(player_type *player_ptr)
{
    (void)player_ptr;

    uint16_t tmp16u;
    rd_u16b(&tmp16u);
    auto town_count = (int)tmp16u;

    rd_u16b(&tmp16u);
    auto store_count = (int)tmp16u;

    for (int town_idx = 1; town_idx < town_count; town_idx++)
        for (int store_idx = 0; store_idx < store_count; store_idx++)
            if (rd_store(player_ptr, town_idx, store_idx))
                return 22;

    return 0;
}
