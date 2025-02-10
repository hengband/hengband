#pragma once

#include "system/angband.h"

class Direction;
class PlayerType;
bool hypodynamic_bolt(PlayerType *player_ptr, DIRECTION dir, int dam);
bool hypodynamic_bolt(PlayerType *player_ptr, const Direction &dir, int dam);
bool death_ray(PlayerType *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
