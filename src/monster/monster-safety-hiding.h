#pragma once

#include "system/angband.h"
#include "monster/monster-util.h"

bool find_safety(player_type *target_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp);
bool find_hiding(player_type *target_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp);
