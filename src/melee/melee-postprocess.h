#pragma once

#include "system/angband.h"
#include "combat/combat-options-type.h"

class PlayerType;
void mon_take_hit_mon(PlayerType *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *dead, bool *fear, concptr note, MONSTER_IDX who);
