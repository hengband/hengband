#pragma once

#include "system/angband.h"

struct player_type;
bool hypodynamic_bolt(player_type *player_ptr, DIRECTION dir, HIT_POINT dam);
bool death_ray(player_type *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
