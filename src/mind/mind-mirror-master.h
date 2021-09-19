#pragma once

#include "system/angband.h"

struct player_type;
bool check_multishadow(player_type *player_ptr);
bool mirror_concentration(player_type *player_ptr);
void remove_all_mirrors(player_type *player_ptr, bool explode);
bool binding_field(player_type *player_ptr, HIT_POINT dam);
void seal_of_mirror(player_type *player_ptr, HIT_POINT dam);
bool confusing_light(player_type *player_ptr);
bool place_mirror(player_type *player_ptr);
bool mirror_tunnel(player_type *player_ptr);
bool set_multishadow(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_dustrobe(player_type *player_ptr, TIME_EFFECT v, bool do_dec);

enum mind_mirror_master_type : int;
bool cast_mirror_spell(player_type *player_ptr, mind_mirror_master_type spell);
