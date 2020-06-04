#pragma once

#include "system/angband.h"

bool check_multishadow(player_type *creature_ptr);
bool mirror_concentration(player_type *creature_ptr);
void remove_all_mirrors(player_type *caster_ptr, bool explode);
bool binding_field(player_type *caster_ptr, HIT_POINT dam);
void seal_of_mirror(player_type *caster_ptr, HIT_POINT dam);
bool confusing_light(player_type *creature_ptr);
