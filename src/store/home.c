#include "store/home.h"
#include "floor/floor-town.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "object/object-generator.h"
#include "player/avatar.h"
#include "store/store-util.h"
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
int home_carry(player_type *player_ptr, object_type *o_ptr)
{
    bool old_stack_force_notes = stack_force_notes;
    bool old_stack_force_costs = stack_force_costs;
    if (cur_store_num != STORE_HOME) {
        stack_force_notes = FALSE;
        stack_force_costs = FALSE;
    }

    for (int slot = 0; slot < st_ptr->stock_num; slot++) {
        object_type *j_ptr;
        j_ptr = &st_ptr->stock[slot];
        if (object_similar(j_ptr, o_ptr)) {
            object_absorb(j_ptr, o_ptr);
            if (cur_store_num != STORE_HOME) {
                stack_force_notes = old_stack_force_notes;
                stack_force_costs = old_stack_force_costs;
            }

            return (slot);
        }
    }

    if (cur_store_num != STORE_HOME) {
        stack_force_notes = old_stack_force_notes;
        stack_force_costs = old_stack_force_costs;
    }

    /* No space? */
    /*
     * 隠し機能: オプション powerup_home が設定されていると
     *           我が家が 20 ページまで使える
     */
    if ((cur_store_num != STORE_HOME) || (powerup_home == TRUE)) {
        if (st_ptr->stock_num >= st_ptr->stock_size) {
            return -1;
        }
    } else {
        if (st_ptr->stock_num >= ((st_ptr->stock_size) / 10)) {
            return -1;
        }
    }

    PRICE value = object_value(player_ptr, o_ptr);
    int slot;
    for (slot = 0; slot < st_ptr->stock_num; slot++)
        if (object_sort_comp(player_ptr, o_ptr, value, &st_ptr->stock[slot]))
            break;

    for (int i = st_ptr->stock_num; i > slot; i--)
        st_ptr->stock[i] = st_ptr->stock[i - 1];

    st_ptr->stock_num++;
    st_ptr->stock[slot] = *o_ptr;
    chg_virtue(player_ptr, V_SACRIFICE, -1);
    (void)combine_and_reorder_home(player_ptr, cur_store_num);
    return slot;
}

/*!
 * @brief 現在の町の指定された店舗のアイテムを整理する /
 * Combine and reorder items in store.
 * @param store_num 店舗ID
 * @return 実際に整理が行われたならばTRUEを返す。
 */
bool combine_and_reorder_home(player_type *player_ptr, int store_num)
{
    bool old_stack_force_notes = stack_force_notes;
    bool old_stack_force_costs = stack_force_costs;
    store_type *old_st_ptr = st_ptr;
    st_ptr = &town_info[1].store[store_num];
    bool flag = FALSE;
    if (store_num != STORE_HOME) {
        stack_force_notes = FALSE;
        stack_force_costs = FALSE;
    }

    bool combined = TRUE;
    while (combined) {
        combined = FALSE;
        for (int i = st_ptr->stock_num - 1; i > 0; i--) {
            object_type *o_ptr;
            o_ptr = &st_ptr->stock[i];
            if (!o_ptr->k_idx)
                continue;

            for (int j = 0; j < i; j++) {
                object_type *j_ptr;
                j_ptr = &st_ptr->stock[j];
                if (!j_ptr->k_idx)
                    continue;

                /*
                 * Get maximum number of the stack if these
                 * are similar, get zero otherwise.
                 */
                int max_num = object_similar_part(j_ptr, o_ptr);
                if (max_num == 0 || j_ptr->number >= max_num)
                    continue;

                if (o_ptr->number + j_ptr->number <= max_num) {
                    object_absorb(j_ptr, o_ptr);
                    st_ptr->stock_num--;
                    int k;
                    for (k = i; k < st_ptr->stock_num; k++) {
                        st_ptr->stock[k] = st_ptr->stock[k + 1];
                    }

                    object_wipe(&st_ptr->stock[k]);
                    combined = TRUE;
                    break;
                }

                ITEM_NUMBER old_num = o_ptr->number;
                ITEM_NUMBER remain = j_ptr->number + o_ptr->number - max_num;
                object_absorb(j_ptr, o_ptr);
                o_ptr->number = remain;
                if (o_ptr->tval == TV_ROD) {
                    o_ptr->pval = o_ptr->pval * remain / old_num;
                    o_ptr->timeout = o_ptr->timeout * remain / old_num;
                } else if (o_ptr->tval == TV_WAND) {
                    o_ptr->pval = o_ptr->pval * remain / old_num;
                }

                combined = TRUE;
                break;
            }
        }

        flag |= combined;
    }

    for (int i = 0; i < st_ptr->stock_num; i++) {
        object_type *o_ptr;
        o_ptr = &st_ptr->stock[i];
        if (!o_ptr->k_idx)
            continue;

        s32b o_value = object_value(player_ptr, o_ptr);
        int j;
        for (j = 0; j < st_ptr->stock_num; j++)
            if (object_sort_comp(player_ptr, o_ptr, o_value, &st_ptr->stock[j]))
                break;

        if (j >= i)
            continue;

        flag = TRUE;
        object_type *j_ptr;
        object_type forge;
        j_ptr = &forge;
        object_copy(j_ptr, &st_ptr->stock[i]);
        for (int k = i; k > j; k--)
            object_copy(&st_ptr->stock[k], &st_ptr->stock[k - 1]);

        object_copy(&st_ptr->stock[j], j_ptr);
    }

    st_ptr = old_st_ptr;
    if (store_num != STORE_HOME) {
        stack_force_notes = old_stack_force_notes;
        stack_force_costs = old_stack_force_costs;
    }

    return flag;
}
