#pragma once

#include "system/angband.h"

struct player_type;
bool charm_monster(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool control_one_undead(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool control_one_demon(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool charm_animal(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev);
