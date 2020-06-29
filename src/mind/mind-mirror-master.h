#pragma once

#include "system/angband.h"

bool check_multishadow(player_type *creature_ptr);
bool mirror_concentration(player_type *creature_ptr);
void remove_all_mirrors(player_type *caster_ptr, bool explode);
bool binding_field(player_type *caster_ptr, HIT_POINT dam);
void seal_of_mirror(player_type *caster_ptr, HIT_POINT dam);
bool confusing_light(player_type *creature_ptr);
bool place_mirror(player_type *caster_ptr);
bool mirror_tunnel(player_type *caster_ptr);
bool set_multishadow(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_dustrobe(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
