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
#include "system/item-entity.h"
#include "system/player-type-definition.h"

/*!
 * @brief 該当のマスに存在している箱のオブジェクトIDを返す。
 * @param y 走査対象にしたいマスのY座標
 * @param x 走査対象にしたいマスのX座標
 * @param trapped TRUEならばトラップが存在する箱のみ、FALSEならば空でない箱全てを対象にする
 * @return 箱が存在する場合そのオブジェクトID、存在しない場合0を返す。
 */
short chest_check(FloorType *floor_ptr, const Pos2D &pos, bool trapped)
{
    auto *g_ptr = &floor_ptr->get_grid(pos);
    for (const auto this_o_idx : g_ptr->o_idx_list) {
        const auto &item = floor_ptr->o_list[this_o_idx];
        const auto is_empty = trapped || (item.pval == 0);
        const auto trapped_only = trapped && (item.pval > 0);
        if ((item.bi_key.tval() == ItemKindType::CHEST) && (!is_empty || trapped_only)) {
            return this_o_idx;
        }
    }

    return 0;
}

/*!
 * @brief プレイヤーの周辺9マスに箱のあるマスがいくつあるかを返す
 * @param trapped TRUEならばトラップの存在が判明している箱のみ対象にする
 * @return 該当する地形の数と、該当するマスの中から1つの座標
 */
std::pair<int, Pos2D> count_chests(PlayerType *player_ptr, bool trapped)
{
    Pos2D pos(0, 0);
    auto count = 0;
    for (auto d = 0; d < 9; d++) {
        const Pos2D pos_neighbor(player_ptr->y + ddy_ddd[d], player_ptr->x + ddx_ddd[d]);
        const auto o_idx = chest_check(player_ptr->current_floor_ptr, pos_neighbor, false);
        if (o_idx == 0) {
            continue;
        }

        const auto &item = player_ptr->current_floor_ptr->o_list[o_idx];
        if (item.pval == 0) {
            continue;
        }

        if (trapped && (!item.is_known() || ((item.pval > 0) && chest_traps[item.pval].none()))) {
            continue;
        }

        ++count;
        pos = pos_neighbor;
    }

    return { count, pos };
}
