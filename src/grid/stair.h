#pragma once

#include "system/angband.h"

struct floor_type;
class PlayerType;
void place_random_stairs(PlayerType *player_ptr, POSITION y, POSITION x);
bool cave_valid_bold(floor_type *floor_ptr, POSITION y, POSITION x);
