/*!
 * @file open-util.cpp
 * @brief 開閉処理関連関数
 */

#include "action/open-util.h"
#include "floor/geometry.h"
#include "grid/trap.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief 該当のマスに存在している箱のオブジェクトIDを返す。
 * @param y 走査対象にしたいマスのY座標
 * @param x 走査対象にしたいマスのX座標
 * @param trapped TRUEならばトラップが存在する箱のみ、FALSEならば空でない箱全てを対象にする
 * @return 箱が存在する場合そのオブジェクトID、存在しない場合0を返す。
 */
OBJECT_IDX chest_check(floor_type *floor_ptr, POSITION y, POSITION x, bool trapped)
{
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    for (const auto this_o_idx : g_ptr->o_idx_list) {
        ObjectType *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        if ((o_ptr->tval == ItemKindType::CHEST) && (((!trapped) && (o_ptr->pval)) || /* non empty */
                                                        ((trapped) && (o_ptr->pval > 0)))) { /* trapped only */
            return this_o_idx;
        }
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
int count_chests(PlayerType *player_ptr, POSITION *y, POSITION *x, bool trapped)
{
    int count = 0;
    for (DIRECTION d = 0; d < 9; d++) {
        POSITION yy = player_ptr->y + ddy_ddd[d];
        POSITION xx = player_ptr->x + ddx_ddd[d];
        OBJECT_IDX o_idx = chest_check(player_ptr->current_floor_ptr, yy, xx, false);
        if (!o_idx) {
            continue;
        }

        ObjectType *o_ptr;
        o_ptr = &player_ptr->current_floor_ptr->o_list[o_idx];
        if (o_ptr->pval == 0) {
            continue;
        }

        if (trapped && (!o_ptr->is_known() || ((o_ptr->pval > 0) && chest_traps[o_ptr->pval].none()))) {
            continue;
        }

        ++count;
        *y = yy;
        *x = xx;
    }

    return count;
}
