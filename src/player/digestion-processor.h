#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
void starve_player(player_type *creature_ptr);
bool set_food(player_type *creature_ptr, TIME_EFFECT v);
