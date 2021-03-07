#include "monster-floor/monster-lite-util.h"
#include "grid/grid.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"

monster_lite_type *initialize_monster_lite_type(floor_type *floor_ptr, monster_lite_type *ml_ptr, monster_type *m_ptr)
{
    ml_ptr->mon_fx = m_ptr->fx;
    ml_ptr->mon_fy = m_ptr->fy;
    ml_ptr->mon_invis = !(floor_ptr->grid_array[ml_ptr->mon_fy][ml_ptr->mon_fx].info & CAVE_VIEW);
    return ml_ptr;
}
