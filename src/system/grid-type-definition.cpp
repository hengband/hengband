#include "system/grid-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 指定座標がFLOOR属性を持ったマスかどうかを返す
 * @param Y 指定Y座標
 * @param X 指定X座標
 * @return FLOOR属性を持っているならばTRUE
 */
bool grid_type::is_floor()
{
    return any_bits(this->info, CAVE_FLOOR);
}

bool grid_type::is_room()
{
    return any_bits(this->info, CAVE_ROOM);
}

bool grid_type::is_extra()
{
    return any_bits(this->info, CAVE_EXTRA);
}

bool grid_type::is_inner()
{
    return any_bits(this->info, CAVE_INNER);
}

bool grid_type::is_outer()
{
    return any_bits(this->info, CAVE_OUTER);
}

bool grid_type::is_solid()
{
    return any_bits(this->info, CAVE_SOLID);
}

bool grid_type::is_icky()
{
    return any_bits(this->info, CAVE_ICKY);
}

bool grid_type::is_lite()
{
    return any_bits(this->info, CAVE_LITE);
}

bool grid_type::is_redraw()
{
    return any_bits(this->info, CAVE_REDRAW);
}

bool grid_type::is_view()
{
    return any_bits(this->info, CAVE_VIEW);
}

bool grid_type::is_object()
{
    return any_bits(this->info, CAVE_OBJECT);
}

bool grid_type::is_mark()
{
    return any_bits(this->info, CAVE_MARK);
}
