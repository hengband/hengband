#pragma once

#include "system/angband.h"

struct player_type;
bool process_quantum_effect(player_type *player_ptr, MONSTER_IDX m_idx, bool see_m);
