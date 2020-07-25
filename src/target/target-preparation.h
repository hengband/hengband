#pragma once

#include "system/angband.h"

bool target_able(player_type *creature_ptr, MONSTER_IDX m_idx);
int get_max_range(player_type *creature_ptr);
void target_set_prepare(player_type *creature_ptr, BIT_FLAGS mode);
