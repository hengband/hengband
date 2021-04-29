#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
void take_turn(player_type *creature_ptr, PERCENTAGE need_cost);
void free_turn(player_type *creature_ptr);
