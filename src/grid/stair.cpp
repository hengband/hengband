#include "grid/stair.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

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
    grid_type *g_ptr;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (!g_ptr->is_floor() || !g_ptr->o_idx_list.empty()) {
        return;
    }

    if (!floor_ptr->dun_level) {
        up_stairs = false;
    }

    if (ironman_downward) {
        up_stairs = false;
    }

    if (floor_ptr->dun_level >= dungeons_info[player_ptr->dungeon_idx].maxdepth) {
        down_stairs = false;
    }

    if (inside_quest(quest_number(player_ptr, floor_ptr->dun_level)) && (floor_ptr->dun_level > 1)) {
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
        set_cave_feat(floor_ptr, y, x, feat_up_stair);
    } else if (down_stairs) {
        set_cave_feat(floor_ptr, y, x, feat_down_stair);
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
bool cave_valid_bold(floor_type *floor_ptr, POSITION y, POSITION x)
{
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->cave_has_flag(FloorFeatureType::PERMANENT)) {
        return false;
    }

    for (const auto this_o_idx : g_ptr->o_idx_list) {
        ObjectType *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        if (o_ptr->is_artifact()) {
            return false;
        }
    }

    return true;
}
