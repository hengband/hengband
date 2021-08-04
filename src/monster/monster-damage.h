#pragma once

#include "system/angband.h"

struct player_type;
struct monster_type;
void get_exp_from_mon(player_type *target_ptr, HIT_POINT dam, monster_type *m_ptr);
