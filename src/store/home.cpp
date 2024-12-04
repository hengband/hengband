#include "store/home.h"
#include "avatar/avatar.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "store/store-util.h"
#include "system/floor/town-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/object-sort.h"
#include <algorithm>

/*!
 * @brief 我が家にオブジェクトを加える /
 * Add the item "o_ptr" to the inventory of the "Home"
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
int home_carry(PlayerType *player_ptr, ItemEntity *o_ptr, StoreSaleType store_num)
{
    bool old_stack_force_notes = stack_force_notes;
    bool old_stack_force_costs = stack_force_costs;
    if (store_num != StoreSaleType::HOME) {
        stack_force_notes = false;
        stack_force_costs = false;
    }

    for (int slot = 0; slot < st_ptr->stock_num; slot++) {
        auto &item_store = *st_ptr->stock[slot];
        if (item_store.is_similar(*o_ptr)) {
            item_store.absorb(*o_ptr);
            if (store_num != StoreSaleType::HOME) {
                stack_force_notes = old_stack_force_notes;
                stack_force_costs = old_stack_force_costs;
            }

            return slot;
        }
    }

    if (store_num != StoreSaleType::HOME) {
        stack_force_notes = old_stack_force_notes;
        stack_force_costs = old_stack_force_costs;
    }

    /* No space? */
    /*
     * 隠し機能: オプション powerup_home が設定されていると
     *           我が家が 20 ページまで使える
     */
    if ((store_num != StoreSaleType::HOME) || powerup_home) {
        if (st_ptr->stock_num >= st_ptr->stock_size) {
            return -1;
        }
    } else {
        if (st_ptr->stock_num >= ((st_ptr->stock_size) / 10)) {
            return -1;
        }
    }

    const auto first = st_ptr->stock.begin();
    const auto last = st_ptr->stock.begin() + st_ptr->stock_num;
    const auto slot_it = std::find_if(first, last,
        [&](const auto &item) { return object_sort_comp(player_ptr, *o_ptr, *item); });
    const int slot = std::distance(first, slot_it);

    std::rotate(first + slot, last, last + 1);

    st_ptr->stock_num++;
    *st_ptr->stock[slot] = *o_ptr;
    chg_virtue(player_ptr, Virtue::SACRIFICE, -1);
    (void)combine_and_reorder_home(player_ptr, store_num);
    return slot;
}

static bool exe_combine_store_items(ItemEntity &item1, ItemEntity &item2, const int max_num, const int i)
{
    if (item1.number + item2.number > max_num) {
        return false;
    }

    item2.absorb(item1);
    const auto begin = st_ptr->stock.begin();
    std::rotate(begin + i, begin + i + 1, begin + st_ptr->stock_num);

    st_ptr->stock_num--;
    st_ptr->stock[st_ptr->stock_num]->wipe();
    return true;
}

static bool sweep_reorder_store_item(ItemEntity &item, const int i)
{
    for (auto j = 0; j < i; j++) {
        auto &item_store = *st_ptr->stock[j];
        if (!item_store.is_valid()) {
            continue;
        }

        const auto max_num = item_store.is_similar_part(item);
        if (max_num == 0 || item_store.number >= max_num) {
            continue;
        }

        if (exe_combine_store_items(item, item_store, max_num, i)) {
            return true;
        }

        const auto old_num = item.number;
        const auto remain = item_store.number + item.number - max_num;
        item_store.absorb(item);
        item.number = remain;
        const auto tval = item.bi_key.tval();
        if (tval == ItemKindType::ROD) {
            item.pval = item.pval * remain / old_num;
            item.timeout = item.timeout * remain / old_num;
        } else if (tval == ItemKindType::WAND) {
            item.pval = item.pval * remain / old_num;
        }

        return true;
    }

    return false;
}

static bool exe_reorder_store_item(PlayerType *player_ptr)
{
    const auto comp = [player_ptr](const auto &item1, const auto &item2) {
        return object_sort_comp(player_ptr, *item1, *item2);
    };

    const auto first = st_ptr->stock.begin();
    const auto last = st_ptr->stock.begin() + st_ptr->stock_num;

    if (std::is_sorted(first, last, comp)) {
        return false;
    }

    std::stable_sort(first, last, comp);
    return true;
}

/*!
 * @brief 現在の町の指定された店舗のアイテムを整理する /
 * Combine and reorder items in store.
 * @param store_num 店舗ID
 * @return 実際に整理が行われたならばTRUEを返す。
 */
bool combine_and_reorder_home(PlayerType *player_ptr, const StoreSaleType store_num)
{
    auto old_stack_force_notes = stack_force_notes;
    auto old_stack_force_costs = stack_force_costs;
    auto *old_st_ptr = st_ptr;
    st_ptr = &towns_info[1].stores[store_num];
    auto flag = false;
    if (store_num != StoreSaleType::HOME) {
        stack_force_notes = false;
        stack_force_costs = false;
    }

    auto combined = true;
    while (combined) {
        combined = false;
        for (auto i = st_ptr->stock_num - 1; i > 0; i--) {
            auto &item = *st_ptr->stock[i];
            if (!item.is_valid()) {
                continue;
            }

            combined |= sweep_reorder_store_item(item, i);
        }

        flag |= combined;
    }

    flag |= exe_reorder_store_item(player_ptr);

    st_ptr = old_st_ptr;
    if (store_num != StoreSaleType::HOME) {
        stack_force_notes = old_stack_force_notes;
        stack_force_costs = old_stack_force_costs;
    }

    return flag;
}
