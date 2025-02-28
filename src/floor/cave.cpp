/*!
 * @brief ダンジョンの壁等に関する判定関数の集合
 * @date 2020/07/18
 * @author Hourier
 * @details
 * Dance with a colorless dress. Shout with a withered voice.
 */

#include "floor/cave.h"
#include "grid/grid.h"
#include "system/angband-system.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

/*
 * @brief 指定のマスが光を通すか(LOSフラグを持つか)を返す。 / Aux function -- see below
 * @param floor フロアへの参照
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return 光を通すならばtrueを返す。
 */
bool cave_los_bold(const FloorType &floor, int y, int x)
{
    return floor.get_grid({ y, x }).has_los_terrain();
}

/*
 * Determine if a "legal" grid is a "clean" floor grid
 * Determine if terrain-change spells are allowed in a grid.
 *
 * Line 1 -- forbid non-floors
 * Line 2 -- forbid object terrains
 * Line 3 -- forbid normal objects
 */
bool cave_clean_bold(const FloorType &floor, int y, int x)
{
    const Pos2D pos(y, x);
    const auto &grid = floor.get_grid(pos);
    return floor.has_terrain_characteristics(pos, TerrainCharacteristics::FLOOR) && !grid.is_object() && grid.o_idx_list.empty();
}

/*
 * Determine if an object can be dropped on a "legal" grid
 *
 * Line 1 -- forbid non-drops
 * Line 2 -- forbid object terrains
 */
bool cave_drop_bold(const FloorType &floor, int y, int x)
{
    const Pos2D pos(y, x);
    const auto &grid = floor.get_grid(pos);
    return floor.has_terrain_characteristics(pos, TerrainCharacteristics::DROP) && !grid.is_object();
}

bool pattern_tile(const FloorType &floor, int y, int x)
{
    const Pos2D pos(y, x);
    return floor.has_terrain_characteristics(pos, TerrainCharacteristics::PATTERN);
}
