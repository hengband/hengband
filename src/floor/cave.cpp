/*!
 * @brief ダンジョンの壁等に関する判定関数の集合
 * @date 2020/07/18
 * @author Hourier
 * @details
 * Dance with a colorless dress. Shout with a withered voice.
 */

#include "floor/cave.h"
#include "floor/floor-list.h"
#include "grid/grid.h"
#include "system/angband-system.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

/*
 * Determines if a map location is fully inside the outer walls
 */
bool in_bounds(const FloorType *floor_ptr, int y, int x)
{
    return (y > 0) && (x > 0) && (y < floor_ptr->height - 1) && (x < floor_ptr->width - 1);
}

/*
 * Determines if a map location is on or inside the outer walls
 */
bool in_bounds2(const FloorType *floor_ptr, int y, int x)
{
    return (y >= 0) && (x >= 0) && (y < floor_ptr->height) && (x < floor_ptr->width);
}

/*
 * Determines if a map location is on or inside the outer walls
 * (unsigned version)
 */
bool in_bounds2u(const FloorType *floor_ptr, int y, int x)
{
    return (y < floor_ptr->height) && (x < floor_ptr->width);
}

/*
 * Determine if a "legal" grid is an "empty" floor grid
 * Determine if monsters are allowed to move into a grid
 *
 * Line 1 -- forbid non-placement grids
 * Line 2 -- forbid normal monsters
 * Line 3 -- forbid the player
 */
bool is_cave_empty_bold(PlayerType *player_ptr, int y, int x)
{
    auto *floor_ptr = &FloorList::get_instance().get_floor(0);
    bool is_empty_grid = cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::PLACE);
    is_empty_grid &= !(floor_ptr->grid_array[y][x].m_idx);
    is_empty_grid &= !player_ptr->is_located_at({ y, x });
    return is_empty_grid;
}

/*
 * Determine if a "legal" grid is an "empty" floor grid
 * Determine if monster generation is allowed in a grid
 *
 * Line 1 -- forbid non-empty grids
 * Line 2 -- forbid trees while dungeon generation
 */
bool is_cave_empty_bold2(PlayerType *player_ptr, int y, int x)
{
    const auto &floor = FloorList::get_instance().get_floor(0);
    bool is_empty_grid = is_cave_empty_bold(player_ptr, y, x);
    is_empty_grid &= AngbandWorld::get_instance().character_dungeon || !cave_has_flag_bold(&floor, y, x, TerrainCharacteristics::TREE);
    return is_empty_grid;
}

bool cave_has_flag_bold(const FloorType *floor_ptr, int y, int x, TerrainCharacteristics f_idx)
{
    const Pos2D pos(y, x);
    return floor_ptr->get_grid(pos).get_terrain().flags.has(f_idx);
}

/*
 * Does the grid stop disintegration?
 */
bool cave_stop_disintegration(const FloorType *floor_ptr, int y, int x)
{
    const auto can_stop = !cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::PROJECT);
    auto is_bold = !cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::HURT_DISI);
    is_bold |= cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::PERMANENT);
    return can_stop && is_bold;
}

/*
 * @brief 指定のマスが光を通すか(LOSフラグを持つか)を返す。 / Aux function -- see below
 * @param floor_ptr 配置するフロアの参照ポインタ
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return 光を通すならばtrueを返す。
 */
bool cave_los_bold(const FloorType *floor_ptr, int y, int x)
{
    return feat_supports_los(floor_ptr->grid_array[y][x].feat);
}

/*
 * Determine if a "feature" supports "los"
 */
bool feat_supports_los(short f_idx)
{
    return TerrainList::get_instance().get_terrain(f_idx).flags.has(TerrainCharacteristics::LOS);
}

/*
 * Determine if a "legal" grid is a "clean" floor grid
 * Determine if terrain-change spells are allowed in a grid.
 *
 * Line 1 -- forbid non-floors
 * Line 2 -- forbid object terrains
 * Line 3 -- forbid normal objects
 */
bool cave_clean_bold(const FloorType *floor_ptr, int y, int x)
{
    return cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::FLOOR) && ((floor_ptr->grid_array[y][x].is_object()) == 0) && floor_ptr->grid_array[y][x].o_idx_list.empty();
}

/*
 * Determine if an object can be dropped on a "legal" grid
 *
 * Line 1 -- forbid non-drops
 * Line 2 -- forbid object terrains
 */
bool cave_drop_bold(const FloorType *floor_ptr, int y, int x)
{
    return cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::DROP) && ((floor_ptr->grid_array[y][x].is_object()) == 0);
}

bool pattern_tile(const FloorType *floor_ptr, int y, int x)
{
    return cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::PATTERN);
}
