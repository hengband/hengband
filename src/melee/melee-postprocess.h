#pragma once

#include "combat/combat-options-type.h"
#include "system/angband.h"

class PlayerType;
void mon_take_hit_mon(PlayerType *player_ptr, MONSTER_IDX m_idx, int dam, bool *dead, bool *fear, concptr note, MONSTER_IDX who);
