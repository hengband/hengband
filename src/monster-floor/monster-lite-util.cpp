#include "monster-floor/monster-lite-util.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "util/bit-flags-calculator.h"

monster_lite_type::monster_lite_type(uint32_t grid_info, const MonsterEntity &monster)
    : mon_invis(none_bits(grid_info, CAVE_VIEW))
    , m_pos(monster.get_position())
{
}
