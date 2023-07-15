#pragma once

#include "system/angband.h"

struct monster_lite_type {
    bool mon_invis;
    POSITION mon_fy;
    POSITION mon_fx;
};

class MonsterEntity;
monster_lite_type *initialize_monster_lite_type(BIT_FLAGS grid_info, monster_lite_type *ml_ptr, MonsterEntity *m_ptr);
