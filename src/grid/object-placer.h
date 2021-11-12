#pragma once

#include "system/angband.h"

class PlayerType;
void place_gold(PlayerType *player_ptr, POSITION y, POSITION x);
void place_object(PlayerType *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode);
