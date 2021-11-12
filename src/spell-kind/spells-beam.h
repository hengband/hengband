#pragma once

#include "system/angband.h"

class PlayerType;
bool wall_to_mud(PlayerType *player_ptr, DIRECTION dir, HIT_POINT dam);
bool wizard_lock(PlayerType *player_ptr, DIRECTION dir);
bool destroy_door(PlayerType *player_ptr, DIRECTION dir);
bool disarm_trap(PlayerType *player_ptr, DIRECTION dir);
