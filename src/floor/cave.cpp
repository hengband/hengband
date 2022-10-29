/*!
 * @brief ダンジョンの壁等に関する判定関数の集合
 * @date 2020/07/18
 * @author Hourier
 * @details
 * Dance with a colorless dress. Shout with a withered voice.
 */

#include "floor/cave.h"
#include "grid/grid.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

/*
 * Determines if a map location is fully inside the outer walls
 */
bool in_bounds(FloorType *floor_ptr, POSITION y, POSITION x)
{
    return (y > 0) && (x > 0) && (y < floor_ptr->height - 1) && (x < floor_ptr->width - 1);
}

/*
 * Determines if a map location is on or inside the outer walls
 */
bool in_bounds2(FloorType *floor_ptr, POSITION y, POSITION x)
{
    return (y >= 0) && (x >= 0) && (y < floor_ptr->height) && (x < floor_ptr->width);
}

/*
 * Determines if a map location is on or inside the outer walls
 * (unsigned version)
 */
bool in_bounds2u(FloorType *floor_ptr, POSITION y, POSITION x)
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
bool is_cave_empty_bold(PlayerType *player_ptr, POSITION y, POSITION x)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    bool is_empty_grid = cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::PLACE);
    is_empty_grid &= !(floor_ptr->grid_array[y][x].m_idx);
    is_empty_grid &= !player_bold(player_ptr, y, x);
    return is_empty_grid;
}

/*
 * Determine if a "legal" grid is an "empty" floor grid
 * Determine if monster generation is allowed in a grid
 *
 * Line 1 -- forbid non-empty grids
 * Line 2 -- forbid trees while dungeon generation
 */
bool is_cave_empty_bold2(PlayerType *player_ptr, POSITION y, POSITION x)
{
    bool is_empty_grid = is_cave_empty_bold(player_ptr, y, x);
    is_empty_grid &= w_ptr->character_dungeon || !cave_has_flag_bold(player_ptr->current_floor_ptr, y, x, TerrainCharacteristics::TREE);
    return is_empty_grid;
}

bool cave_has_flag_bold(FloorType *floor_ptr, POSITION y, POSITION x, TerrainCharacteristics f_idx)
{
    return terrains_info[floor_ptr->grid_array[y][x].feat].flags.has(f_idx);
}

/*
 * Determine if a "legal" grid is within "los" of the player
 */
bool player_has_los_bold(PlayerType *player_ptr, POSITION y, POSITION x)
{
    return ((player_ptr->current_floor_ptr->grid_array[y][x].info & CAVE_VIEW) != 0) || player_ptr->phase_out;
}

/*
 * Determine if player is on this grid
 */
bool player_bold(PlayerType *player_ptr, POSITION y, POSITION x)
{
    return (y == player_ptr->y) && (x == player_ptr->x);
}

/*
 * Does the grid stop disintegration?
 */
bool cave_stop_disintegration(FloorType *floor_ptr, POSITION y, POSITION x)
{
    return !cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::PROJECT) && (!cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::HURT_DISI) || cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::PERMANENT));
}

/*
 * @brief 指定のマスが光を通すか(LOSフラグを持つか)を返す。 / Aux function -- see below
 * @param floor_ptr 配置するフロアの参照ポインタ
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return 光を通すならばtrueを返す。
 */
bool cave_los_bold(FloorType *floor_ptr, POSITION y, POSITION x)
{
    return feat_supports_los(floor_ptr->grid_array[y][x].feat);
}

/*
 * Determine if a "feature" supports "los"
 */
bool feat_supports_los(FEAT_IDX f_idx)
{
    return terrains_info[f_idx].flags.has(TerrainCharacteristics::LOS);
}

/*
 * Determine if a "legal" grid is a "clean" floor grid
 * Determine if terrain-change spells are allowed in a grid.
 *
 * Line 1 -- forbid non-floors
 * Line 2 -- forbid object terrains
 * Line 3 -- forbid normal objects
 */
bool cave_clean_bold(FloorType *floor_ptr, POSITION y, POSITION x)
{
    return cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::FLOOR) && ((floor_ptr->grid_array[y][x].is_object()) == 0) && floor_ptr->grid_array[y][x].o_idx_list.empty();
}

/*
 * Determine if an object can be dropped on a "legal" grid
 *
 * Line 1 -- forbid non-drops
 * Line 2 -- forbid object terrains
 */
bool cave_drop_bold(FloorType *floor_ptr, POSITION y, POSITION x)
{
    return cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::DROP) && ((floor_ptr->grid_array[y][x].is_object()) == 0);
}

bool pattern_tile(FloorType *floor_ptr, POSITION y, POSITION x)
{
    return cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::PATTERN);
}
