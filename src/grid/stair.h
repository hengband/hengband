#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class FloorType;
class PlayerType;
void place_random_stairs(PlayerType *player_ptr, const Pos2D &pos);
bool cave_valid_bold(FloorType *floor_ptr, POSITION y, POSITION x);
