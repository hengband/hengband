#pragma once

#include "system/angband.h"

struct player_type;
bool earthquake(player_type *caster_ptr, POSITION cy, POSITION cx, POSITION r, MONSTER_IDX m_idx);
