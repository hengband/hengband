#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool make_attack_spell(player_type *target_ptr, MONSTER_IDX m_idx);
