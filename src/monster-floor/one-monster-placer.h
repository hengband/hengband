#pragma once

#include "system/angband.h"

struct player_type;
bool place_monster_one(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode);
