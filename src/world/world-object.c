#include "world/world-object.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "object-enchant/item-apply-magic.h"
#include "object/object-kind.h"
#include "system/alloc-entries.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief グローバルオブジェクト配列から空きを取得する /
 * Acquires and returns the index of a "free" object.
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @return 開いているオブジェクト要素のID
 * @details
 * This routine should almost never fail, but in case it does,
 * we must be sure to handle "failure" of this routine.
 */
OBJECT_IDX o_pop(floor_type *floor_ptr)
{
    if (floor_ptr->o_max < current_world_ptr->max_o_idx) {
        OBJECT_IDX i = floor_ptr->o_max;
        floor_ptr->o_max++;
        floor_ptr->o_cnt++;
        return i;
    }

    for (OBJECT_IDX i = 1; i < floor_ptr->o_max; i++) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[i];
        if (o_ptr->k_idx)
            continue;
        floor_ptr->o_cnt++;

        return i;
    }

    if (current_world_ptr->character_dungeon)
        msg_print(_("アイテムが多すぎる！", "Too many objects!"));

    return 0;
}

/*!
 * @brief オブジェクト生成テーブルからアイテムを取得する /
 * Choose an object kind that seems "appropriate" to the given level
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param level 生成階
 * @return 選ばれたオブジェクトベースID
 * @details
 * This function uses the "prob2" field of the "object allocation table",\n
 * and various local information, to calculate the "prob3" field of the\n
 * same table, which is then used to choose an "appropriate" object, in\n
 * a relatively efficient manner.\n
 *\n
 * It is (slightly) more likely to acquire an object of the given level\n
 * than one of a lower level.  This is done by choosing several objects\n
 * appropriate to the given level and keeping the "hardest" one.\n
 *\n
 * Note that if no objects are "appropriate", then this function will\n
 * fail, and return zero, but this should *almost* never happen.\n
 */
OBJECT_IDX get_obj_num(player_type *owner_ptr, DEPTH level, BIT_FLAGS mode)
{
    int i, j, p;
    KIND_OBJECT_IDX k_idx;
    long value, total;
    object_kind *k_ptr;
    alloc_entry *table = alloc_kind_table;

    if (level > MAX_DEPTH - 1)
        level = MAX_DEPTH - 1;

    if ((level > 0) && !(d_info[owner_ptr->dungeon_idx].flags1 & DF1_BEGINNER)) {
        if (one_in_(GREAT_OBJ)) {
            level = 1 + (level * MAX_DEPTH / randint1(MAX_DEPTH));
        }
    }

    total = 0L;
    for (i = 0; i < alloc_kind_size; i++) {
        if (table[i].level > level)
            break;

        table[i].prob3 = 0;
        k_idx = table[i].index;
        k_ptr = &k_info[k_idx];

        if ((mode & AM_FORBID_CHEST) && (k_ptr->tval == TV_CHEST))
            continue;

        table[i].prob3 = table[i].prob2;
        total += table[i].prob3;
    }

    if (total <= 0)
        return 0;

    value = randint0(total);
    for (i = 0; i < alloc_kind_size; i++) {
        if (value < table[i].prob3)
            break;

        value = value - table[i].prob3;
    }

    p = randint0(100);
    if (p < 60) {
        j = i;
        value = randint0(total);
        for (i = 0; i < alloc_kind_size; i++) {
            if (value < table[i].prob3)
                break;

            value = value - table[i].prob3;
        }

        if (table[i].level < table[j].level)
            i = j;
    }

    if (p >= 10)
        return (table[i].index);

    j = i;
    value = randint0(total);
    for (i = 0; i < alloc_kind_size; i++) {
        if (value < table[i].prob3)
            break;

        value = value - table[i].prob3;
    }

    if (table[i].level < table[j].level)
        i = j;
    return (table[i].index);
}
