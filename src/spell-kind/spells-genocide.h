#pragma once

#include "system/angband.h"

struct player_type;
bool genocide_aux(player_type *caster_ptr, MONSTER_IDX m_idx, int power, bool player_cast, int dam_side, concptr spell_name);
bool symbol_genocide(player_type *caster_ptr, int power, bool player_cast);
bool mass_genocide(player_type *caster_ptr, int power, bool player_cast);
bool mass_genocide_undead(player_type *caster_ptr, int power, bool player_cast);
