#pragma once

#include "system/angband.h"

struct player_type;
bool wall_to_mud(player_type *caster_ptr, DIRECTION dir, HIT_POINT dam);
bool wizard_lock(player_type *caster_ptr, DIRECTION dir);
bool destroy_door(player_type *caster_ptr, DIRECTION dir);
bool disarm_trap(player_type *caster_ptr, DIRECTION dir);
