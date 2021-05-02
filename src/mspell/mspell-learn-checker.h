#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool spell_learnable(player_type *target_ptr, MONSTER_IDX m_idx);
