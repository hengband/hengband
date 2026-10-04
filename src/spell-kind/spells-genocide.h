#pragma once

#include "system/angband.h"
#include <string_view>

class PlayerType;
bool genocide_aux(PlayerType *player_ptr, MONSTER_IDX m_idx, int power, bool player_cast, int dam_side, std::string_view spell_name);
bool symbol_genocide(PlayerType *player_ptr, int power, bool player_cast);
bool mass_genocide(PlayerType *player_ptr, int power, bool player_cast);
bool mass_genocide_undead(PlayerType *player_ptr, int power, bool player_cast);
