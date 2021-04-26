#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

typedef struct player_type player_type;
MonsterSpellResult spell_RF4_DISPEL(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE);
