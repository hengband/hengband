﻿#pragma once

#include "system/angband.h"

class player_type;
bool wall_to_mud(player_type *player_ptr, DIRECTION dir, HIT_POINT dam);
bool wizard_lock(player_type *player_ptr, DIRECTION dir);
bool destroy_door(player_type *player_ptr, DIRECTION dir);
bool disarm_trap(player_type *player_ptr, DIRECTION dir);
