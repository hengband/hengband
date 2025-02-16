#pragma once

#include "system/angband.h"

class Direction;
class PlayerType;
bool hypodynamic_bolt(PlayerType *player_ptr, const Direction &dir, int dam);
bool death_ray(PlayerType *player_ptr, const Direction &dir, PLAYER_LEVEL plev);
