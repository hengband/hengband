#pragma once

#include "system/angband.h"

class PlayerType;
bool check_multishadow(PlayerType *player_ptr);
bool mirror_concentration(PlayerType *player_ptr);
void remove_all_mirrors(PlayerType *player_ptr, bool explode);
bool binding_field(PlayerType *player_ptr, int dam);
void seal_of_mirror(PlayerType *player_ptr, int dam);
bool confusing_light(PlayerType *player_ptr);
bool place_mirror(PlayerType *player_ptr);
bool mirror_tunnel(PlayerType *player_ptr);
bool set_multishadow(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_dustrobe(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);

enum mind_mirror_master_type : int;
bool cast_mirror_spell(PlayerType *player_ptr, mind_mirror_master_type spell);
