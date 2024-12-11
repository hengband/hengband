#include "grid/stair.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"

/*!
 * @brief 所定の位置に上り階段か下り階段を配置する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 配置を試みたいマスの座標
 */
void place_random_stairs(PlayerType *player_ptr, const Pos2D &pos)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    if (!grid.is_floor() || !grid.o_idx_list.empty()) {
        return;
    }

    auto up_stairs = floor.is_in_underground();
    if (ironman_downward) {
        up_stairs = false;
    }

    auto down_stairs = floor.dun_level < floor.get_dungeon_definition().maxdepth;
    if (inside_quest(floor.get_quest_id()) && (floor.dun_level > 1)) {
        down_stairs = false;
    }

    if (down_stairs && up_stairs) {
        if (one_in_(2)) {
            up_stairs = false;
        } else {
            down_stairs = false;
        }
    }

    const auto &terrains = TerrainList::get_instance();
    if (up_stairs) {
        set_cave_feat(&floor, pos.y, pos.x, terrains.get_terrain_id(TerrainTag::UP_STAIR));
        return;
    }
    
    if (down_stairs) {
        set_cave_feat(&floor, pos.y, pos.x, feat_down_stair);
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
