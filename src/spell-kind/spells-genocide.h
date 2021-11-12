#pragma once

#include "system/angband.h"

class PlayerType;
bool genocide_aux(PlayerType *player_ptr, MONSTER_IDX m_idx, int power, bool player_cast, int dam_side, concptr spell_name);
bool symbol_genocide(PlayerType *player_ptr, int power, bool player_cast);
bool mass_genocide(PlayerType *player_ptr, int power, bool player_cast);
bool mass_genocide_undead(PlayerType *player_ptr, int power, bool player_cast);
