#pragma once

#include "system/angband.h"
#include "combat/combat-options-type.h"

struct player_type;
void mon_take_hit_mon(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *dead, bool *fear, concptr note, MONSTER_IDX who);
