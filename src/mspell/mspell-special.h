﻿#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

class player_type;
MonsterSpellResult spell_RF6_SPECIAL(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
