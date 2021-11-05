#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

class PlayerType;
MonsterSpellResult spell_RF4_BREATH(PlayerType *player_ptr, int GF_TYPE, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
