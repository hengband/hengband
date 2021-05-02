#pragma once

#include "system/angband.h"

typedef struct monster_type monster_type;
concptr look_mon_desc(monster_type *m_ptr, BIT_FLAGS mode);
