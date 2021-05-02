#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool get_movable_grid(player_type *target_ptr, MONSTER_IDX m_idx, DIRECTION *mm);
