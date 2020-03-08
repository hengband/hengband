#pragma once

#include "angband.h"
#include "monster/monster-util.h"

bool process_monster_movement(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, DIRECTION *mm, POSITION oy, POSITION ox, int *count);
