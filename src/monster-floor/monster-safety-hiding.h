#pragma once

#include "system/angband.h"

class PlayerType;
bool find_safety(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp);
bool find_hiding(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp);
