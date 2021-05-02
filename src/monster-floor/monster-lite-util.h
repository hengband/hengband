#pragma once

#include "system/angband.h"

typedef struct monster_lite_type {
    bool mon_invis;
    POSITION mon_fy;
    POSITION mon_fx;
} monster_lite_type;

typedef struct floor_type floor_type;
typedef struct monster_type monster_type;
monster_lite_type *initialize_monster_lite_type(floor_type *floor_ptr, monster_lite_type *ml_ptr, monster_type *m_ptr);
