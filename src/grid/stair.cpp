#include "grid/stair.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"

/*!
 * @brief 所定の位置に上り階段か下り階段を配置する / Place an up/down staircase at given location
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 配置を試みたいマスのY座標
 * @param x 配置を試みたいマスのX座標
 */
void place_random_stairs(PlayerType *player_ptr, POSITION y, POSITION x)
{
    bool up_stairs = true;
    bool down_stairs = true;
    auto &floor = *player_ptr->current_floor_ptr;
    const auto *g_ptr = &floor.grid_array[y][x];
    if (!g_ptr->is_floor() || !g_ptr->o_idx_list.empty()) {
        return;
    }

    if (!floor.dun_level) {
        up_stairs = false;
    }

    if (ironman_downward) {
        up_stairs = false;
    }

    if (floor.dun_level >= floor.get_dungeon_definition().maxdepth) {
        down_stairs = false;
    }

    if (inside_quest(floor.get_quest_id()) && (floor.dun_level > 1)) {
        down_stairs = false;
    }

    if (down_stairs && up_stairs) {
        if (randint0(100) < 50) {
            up_stairs = false;
        } else {
            down_stairs = false;
        }
    }

    if (up_stairs) {
        set_cave_feat(&floor, y, x, feat_up_stair);
    } else if (down_stairs) {
        set_cave_feat(&floor, y, x, feat_down_stair);
    }
}

/*!
 * @brief 指定された座標が地震や階段生成の対象となるマスかを返す。 / Determine if a given location may be "destroyed"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y y座標
 * @param x x座標
 * @return 各種の変更が可能ならTRUEを返す。
 * @details
 * 条件は永久地形でなく、なおかつ該当のマスにアーティファクトが存在しないか、である。英語の旧コメントに反して＊破壊＊の抑止判定には現在使われていない。
 */
bool cave_valid_bold(FloorType *floor_ptr, POSITION y, POSITION x)
{
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->cave_has_flag(TerrainCharacteristics::PERMANENT)) {
        return false;
    }

    for (const auto this_o_idx : g_ptr->o_idx_list) {
        ItemEntity *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        if (o_ptr->is_fixed_or_random_artifact()) {
            return false;
        }
    }

    return true;
}
