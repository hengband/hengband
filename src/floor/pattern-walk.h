#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool pattern_effect(player_type *creature_ptr);
bool pattern_seq(player_type *creature_ptr, POSITION c_y, POSITION c_x, POSITION n_y, POSITION n_x);
void pattern_teleport(player_type *creature_ptr);
