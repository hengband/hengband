﻿#include "action/open-util.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "perception/object-perception.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"

/*!
 * @brief 該当のマスに存在している箱のオブジェクトIDを返す。
 * @param y 走査対象にしたいマスのY座標
 * @param x 走査対象にしたいマスのX座標
 * @param trapped TRUEならばトラップが存在する箱のみ、FALSEならば空でない箱全てを対象にする
 * @return 箱が存在する場合そのオブジェクトID、存在しない場合0を返す。
 */
OBJECT_IDX chest_check(floor_type *floor_ptr, POSITION y, POSITION x, bool trapped)
{
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    OBJECT_IDX this_o_idx, next_o_idx = 0;
    for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        if ((o_ptr->tval == TV_CHEST)
            && (((!trapped) && (o_ptr->pval)) || /* non empty */
                ((trapped) && (o_ptr->pval > 0)))) /* trapped only */
            return this_o_idx;
    }

    return 0;
}

/*!
 * @brief プレイヤーの周辺9マスに箱のあるマスがいくつあるかを返す /
 * Return the number of chests around (or under) the character.
 * @param y 該当するマスの中から1つのY座標を返す参照ポインタ
 * @param x 該当するマスの中から1つのX座標を返す参照ポインタ
 * @param trapped TRUEならばトラップの存在が判明している箱のみ対象にする
 * @return 該当する地形の数
 * @details
 * If requested, count only trapped chests.
 */
int count_chests(player_type *creature_ptr, POSITION *y, POSITION *x, bool trapped)
{
    int count = 0;
    for (DIRECTION d = 0; d < 9; d++) {
        POSITION yy = creature_ptr->y + ddy_ddd[d];
        POSITION xx = creature_ptr->x + ddx_ddd[d];
        OBJECT_IDX o_idx = chest_check(creature_ptr->current_floor_ptr, yy, xx, FALSE);
        if (!o_idx)
            continue;

        object_type *o_ptr;
        o_ptr = &creature_ptr->current_floor_ptr->o_list[o_idx];
        if (o_ptr->pval == 0)
            continue;

        if (trapped && (!object_is_known(o_ptr) || !chest_traps[o_ptr->pval]))
            continue;

        ++count;
        *y = yy;
        *x = xx;
    }

    return count;
}
