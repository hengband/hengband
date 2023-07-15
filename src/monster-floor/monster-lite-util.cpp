#include "monster-floor/monster-lite-util.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "util/bit-flags-calculator.h"

monster_lite_type *initialize_monster_lite_type(BIT_FLAGS grid_info, monster_lite_type *ml_ptr, MonsterEntity *m_ptr)
{
    ml_ptr->mon_fx = m_ptr->fx;
    ml_ptr->mon_fy = m_ptr->fy;
    ml_ptr->mon_invis = none_bits(grid_info, CAVE_VIEW);
    return ml_ptr;
}
