#pragma once

#include "system/angband.h"

struct player_type;
bool monst_attack_monst(player_type *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
