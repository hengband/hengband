#pragma once

#include "system/angband.h"

struct player_type;
bool mon_take_hit(player_type *target_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *fear, concptr note);
