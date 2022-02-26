#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

struct MonsterSpellResult;

class PlayerType;
MonsterSpellResult spell_RF4_BREATH(PlayerType *player_ptr, AttributeType GF_TYPE, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
