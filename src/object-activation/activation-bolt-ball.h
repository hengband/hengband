#pragma once

#include "system/angband.h"

struct player_type;
bool activate_missile_1(player_type *player_ptr);
bool activate_missile_2(player_type *player_ptr);
bool activate_missile_3(player_type *player_ptr);
bool activate_bolt_acid_1(player_type *player_ptr);
bool activate_bolt_elec_1(player_type *player_ptr);
bool activate_bolt_fire_1(player_type *player_ptr);
bool activate_bolt_cold_1(player_type *player_ptr);
bool activate_bolt_hypodynamia_1(player_type *player_ptr, concptr name);
bool activate_bolt_hypodynamia_2(player_type *player_ptr);
bool activate_bolt_drain_1(player_type *player_ptr);
bool activate_bolt_drain_2(player_type *player_ptr);
bool activate_bolt_mana(player_type *player_ptr, concptr name);
bool activate_ball_pois_1(player_type *player_ptr);
bool activate_ball_cold_1(player_type *player_ptr);
bool activate_ball_cold_2(player_type *player_ptr);
bool activate_ball_cold_3(player_type *player_ptr);
bool activate_ball_fire_1(player_type *player_ptr);
bool activate_ball_fire_2(player_type *player_ptr, concptr name);
bool activate_ball_fire_3(player_type *player_ptr);
bool activate_ball_fire_4(player_type *player_ptr);
bool activate_ball_elec_2(player_type *player_ptr);
bool activate_ball_elec_3(player_type *player_ptr);
bool activate_ball_acid_1(player_type *player_ptr);
bool activate_ball_nuke_1(player_type *player_ptr);
bool activate_rocket(player_type *player_ptr);
bool activate_ball_water(player_type *player_ptr, concptr name);
bool activate_ball_lite(player_type *player_ptr, concptr name);
bool activate_ball_dark(player_type *player_ptr, concptr name);
bool activate_ball_mana(player_type *player_ptr, concptr name);
