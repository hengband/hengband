#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

class player_type;
MonsterSpellResult spell_RF4_DISPEL(MONSTER_IDX m_idx, player_type *player_ptr, MONSTER_IDX t_idx, int TARGET_TYPE);
