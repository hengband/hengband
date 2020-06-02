#include "world/world-object.h"
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
