#pragma once

#include "system/angband.h"

class player_type;
bool find_safety(player_type *player_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp);
bool find_hiding(player_type *player_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp);
