#include "system/grid-type-definition.h"
#include "util/bit-flags-calculator.h"

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
