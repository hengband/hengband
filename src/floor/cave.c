/*!
 * @brief ダンジョンの壁等に関する判定関数の集合
 * @date 2020/07/18
 * @author Hourier
 * @details
 * Dance with a colorless dress. Shout with a withered voice.
 */

#include "floor/cave.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"

/*
 * Determines if a map location is fully inside the outer walls
 */
bool in_bounds(floor_type *floor_ptr, POSITION y, POSITION x) { return (y > 0) && (x > 0) && (y < floor_ptr->height - 1) && (x < floor_ptr->width - 1); }

/*
 * Determines if a map location is on or inside the outer walls
 */
bool in_bounds2(floor_type *floor_ptr, POSITION y, POSITION x) { return (y >= 0) && (x >= 0) && (y < floor_ptr->height) && (x < floor_ptr->width); }

/*
 * Determines if a map location is on or inside the outer walls
 * (unsigned version)
 */
bool in_bounds2u(floor_type *floor_ptr, POSITION y, POSITION x) { return (y < floor_ptr->height) && (x < floor_ptr->width); }

bool cave_have_flag_bold(floor_type *floor_ptr, POSITION y, POSITION x, feature_flag_type f_idx)
{
    return have_flag(f_info[floor_ptr->grid_array[y][x].feat].flags, f_idx);
}
