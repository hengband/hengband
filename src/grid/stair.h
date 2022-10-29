#pragma once

#include "system/angband.h"

class FloorType;
class PlayerType;
void place_random_stairs(PlayerType *player_ptr, POSITION y, POSITION x);
bool cave_valid_bold(FloorType *floor_ptr, POSITION y, POSITION x);
