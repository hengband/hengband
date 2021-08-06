#include "system/grid-type-definition.h"
#include "util/bit-flags-calculator.h"

bool grid_type::is_floor()
{
    return any_bits(this->info, CAVE_FLOOR);
}
