#pragma once

#include "system/angband.h"
#include "monster/monster-processor-util.h"

bool find_safety(player_type *target_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp);
bool find_hiding(player_type *target_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp);
