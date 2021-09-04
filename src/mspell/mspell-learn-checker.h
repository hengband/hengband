#pragma once

#include "system/angband.h"

struct player_type;
bool spell_learnable(player_type *target_ptr, MONSTER_IDX m_idx);
