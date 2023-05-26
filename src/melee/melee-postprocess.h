#pragma once

#include "combat/combat-options-type.h"
#include "system/angband.h"
#include <string_view>

class PlayerType;
void mon_take_hit_mon(PlayerType *player_ptr, MONSTER_IDX m_idx, int dam, bool *dead, bool *fear, std::string_view note, MONSTER_IDX who);
