#pragma once

#include "system/angband.h"

class PlayerType;
bool place_monster_one(PlayerType *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode);
