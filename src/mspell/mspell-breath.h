#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"

struct MonsterSpellResult;

struct player_type;
MonsterSpellResult spell_RF4_BREATH(player_type *player_ptr, AttributeType GF_TYPE, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
