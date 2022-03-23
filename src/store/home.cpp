#include "store/home.h"
#include "avatar/avatar.h"
#include "floor/floor-town.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "store/store-util.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/object-sort.h"

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
int home_carry(PlayerType *player_ptr, ObjectType *o_ptr, StoreSaleType store_num)
{
    bool old_stack_force_notes = stack_force_notes;
    bool old_stack_force_costs = stack_force_costs;
    if (store_num != StoreSaleType::HOME) {
        stack_force_notes = false;
        stack_force_costs = false;
    }

    for (int slot = 0; slot < st_ptr->stock_num; slot++) {
        ObjectType *j_ptr;
        j_ptr = &st_ptr->stock[slot];
        if (object_similar(j_ptr, o_ptr)) {
            object_absorb(j_ptr, o_ptr);
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

    PRICE value = object_value(o_ptr);
    int slot;
    for (slot = 0; slot < st_ptr->stock_num; slot++) {
        if (object_sort_comp(player_ptr, o_ptr, value, &st_ptr->stock[slot])) {
            break;
        }
    }

    for (int i = st_ptr->stock_num; i > slot; i--) {
        st_ptr->stock[i] = st_ptr->stock[i - 1];
    }

    st_ptr->stock_num++;
    st_ptr->stock[slot] = *o_ptr;
    chg_virtue(player_ptr, V_SACRIFICE, -1);
    (void)combine_and_reorder_home(player_ptr, store_num);
    return slot;
}

static bool exe_combine_store_items(ObjectType *o_ptr, ObjectType *j_ptr, const int max_num, const int i, bool *combined)
{
    if (o_ptr->number + j_ptr->number > max_num) {
        return false;
    }

    object_absorb(j_ptr, o_ptr);
    st_ptr->stock_num--;
    int k;
    for (k = i; k < st_ptr->stock_num; k++) {
        st_ptr->stock[k] = st_ptr->stock[k + 1];
    }

    (&st_ptr->stock[k])->wipe();
    *combined = true;
    return true;
}

static void sweep_reorder_store_item(ObjectType *o_ptr, const int i, bool *combined)
{
    for (int j = 0; j < i; j++) {
        ObjectType *j_ptr;
        j_ptr = &st_ptr->stock[j];
        if (!j_ptr->k_idx) {
            continue;
        }

        int max_num = object_similar_part(j_ptr, o_ptr);
        if (max_num == 0 || j_ptr->number >= max_num) {
            continue;
        }

        if (exe_combine_store_items(o_ptr, j_ptr, max_num, i, combined)) {
            break;
        }

        ITEM_NUMBER old_num = o_ptr->number;
        ITEM_NUMBER remain = j_ptr->number + o_ptr->number - max_num;
        object_absorb(j_ptr, o_ptr);
        o_ptr->number = remain;
        if (o_ptr->tval == ItemKindType::ROD) {
            o_ptr->pval = o_ptr->pval * remain / old_num;
            o_ptr->timeout = o_ptr->timeout * remain / old_num;
        } else if (o_ptr->tval == ItemKindType::WAND) {
            o_ptr->pval = o_ptr->pval * remain / old_num;
        }

        *combined = true;
        break;
    }
}

static void exe_reorder_store_item(PlayerType *player_ptr, bool *flag)
{
    for (int i = 0; i < st_ptr->stock_num; i++) {
        ObjectType *o_ptr;
        o_ptr = &st_ptr->stock[i];
        if (!o_ptr->k_idx) {
            continue;
        }

        int32_t o_value = object_value(o_ptr);
        int j;
        for (j = 0; j < st_ptr->stock_num; j++) {
            if (object_sort_comp(player_ptr, o_ptr, o_value, &st_ptr->stock[j])) {
                break;
            }
        }

        if (j >= i) {
            continue;
        }

        *flag = true;
        ObjectType *j_ptr;
        ObjectType forge;
        j_ptr = &forge;
        j_ptr->copy_from(&st_ptr->stock[i]);
        for (int k = i; k > j; k--) {
            (&st_ptr->stock[k])->copy_from(&st_ptr->stock[k - 1]);
        }

        (&st_ptr->stock[j])->copy_from(j_ptr);
    }
}

/*!
 * @brief 現在の町の指定された店舗のアイテムを整理する /
 * Combine and reorder items in store.
 * @param store_num 店舗ID
 * @return 実際に整理が行われたならばTRUEを返す。
 */
bool combine_and_reorder_home(PlayerType *player_ptr, const StoreSaleType store_num)
{
    bool old_stack_force_notes = stack_force_notes;
    bool old_stack_force_costs = stack_force_costs;
    store_type *old_st_ptr = st_ptr;
    st_ptr = &town_info[1].store[enum2i(store_num)];
    bool flag = false;
    if (store_num != StoreSaleType::HOME) {
        stack_force_notes = false;
        stack_force_costs = false;
    }

    bool combined = true;
    while (combined) {
        combined = false;
        for (int i = st_ptr->stock_num - 1; i > 0; i--) {
            ObjectType *o_ptr;
            o_ptr = &st_ptr->stock[i];
            if (!o_ptr->k_idx) {
                continue;
            }

            sweep_reorder_store_item(o_ptr, i, &combined);
        }

        flag |= combined;
    }

    exe_reorder_store_item(player_ptr, &flag);
    st_ptr = old_st_ptr;
    if (store_num != StoreSaleType::HOME) {
        stack_force_notes = old_stack_force_notes;
        stack_force_costs = old_stack_force_costs;
    }

    return flag;
}
