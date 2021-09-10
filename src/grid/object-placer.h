#pragma once

#include "system/angband.h"

struct player_type;
void place_gold(player_type *player_ptr, POSITION y, POSITION x);
void place_object(player_type *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode);
