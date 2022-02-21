#include "load/store-loader.h"
#include "avatar/avatar.h"
#include "floor/floor-town.h"
#include "load/angband-version-comparer.h"
#include "load/item/item-loader-factory.h"
#include "load/load-util.h"
#include "load/old/item-loader-savefile50.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "store/store.h"
#include "store/store-owners.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/object-sort.h"
#include <stdint.h>

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
static void home_carry_load(PlayerType *player_ptr, store_type *store_ptr, ObjectType *o_ptr)
{
    for (auto i = 0; i < store_ptr->stock_num; i++) {
        auto *j_ptr = &store_ptr->stock[i];
        if (!object_similar(j_ptr, o_ptr)) {
            continue;
        }

        object_absorb(j_ptr, o_ptr);
        return;
    }

    if (store_ptr->stock_num >= store_get_stock_max(StoreSaleType::HOME)) {
        return;
    }

    auto value = object_value(o_ptr);
    int slot;
    for (slot = 0; slot < store_ptr->stock_num; slot++) {
        if (object_sort_comp(player_ptr, o_ptr, value, &store_ptr->stock[slot]))
            break;
    }

    for (auto i = store_ptr->stock_num; i > slot; i--) {
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
static void rd_store(PlayerType *player_ptr, int town_number, int store_number)
{
    store_type *store_ptr;
    auto sort = false;
    if (h_older_than(0, 3, 3) && (i2enum<StoreSaleType>(store_number) == StoreSaleType::HOME)) {
        store_ptr = &town_info[1].store[store_number];
        if (store_ptr->stock_num) {
            sort = true;
        }
    } else {
        store_ptr = &town_info[town_number].store[store_number];
    }

    store_ptr->store_open = rd_s32b();
    store_ptr->insult_cur = rd_s16b();
    store_ptr->owner = rd_byte();

    if (auto num = owners.at(i2enum<StoreSaleType>(store_number)).size();
        num <= store_ptr->owner) {
        store_ptr->owner %= num;
    }

    int16_t inven_num;
    if (h_older_than(1, 0, 4)) {
        inven_num = rd_byte();
    } else {
        inven_num = rd_s16b();
    }

    store_ptr->good_buy = rd_s16b();
    store_ptr->bad_buy = rd_s16b();
    store_ptr->last_visit = rd_s32b();

    auto item_loader = ItemLoaderFactory::create_loader();
    for (int j = 0; j < inven_num; j++) {
        ObjectType item;
        item_loader->rd_item(&item);
        auto stock_max = store_get_stock_max(i2enum<StoreSaleType>(store_number));
        if (store_ptr->stock_num >= stock_max) {
            continue;
        }

        if (sort) {
            home_carry_load(player_ptr, store_ptr, &item);
        } else {
            int k = store_ptr->stock_num++;
            store_ptr->stock[k].copy_from(&item);
        }
    }
}

/*!
 * @brief 店舗情報を読み込む
 * @param player_ptr プレイヤー情報への参照ポインタ(未使用)
 * @return 読み込み終わったら0、失敗したら22
 */
void load_store(PlayerType *player_ptr)
{
    int16_t town_count = rd_u16b();
    int16_t store_count = rd_u16b();
    for (int16_t town_idx = 1; town_idx < town_count; town_idx++) {
        for (int16_t store_idx = 0; store_idx < store_count; store_idx++) {
            rd_store(player_ptr, town_idx, store_idx);
        }
    }
}
