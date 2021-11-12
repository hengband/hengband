#pragma once

#include "system/angband.h"

class PlayerType;
bool activate_missile_1(PlayerType *player_ptr);
bool activate_missile_2(PlayerType *player_ptr);
bool activate_missile_3(PlayerType *player_ptr);
bool activate_bolt_acid_1(PlayerType *player_ptr);
bool activate_bolt_elec_1(PlayerType *player_ptr);
bool activate_bolt_fire_1(PlayerType *player_ptr);
bool activate_bolt_cold_1(PlayerType *player_ptr);
bool activate_bolt_hypodynamia_1(PlayerType *player_ptr, concptr name);
bool activate_bolt_hypodynamia_2(PlayerType *player_ptr);
bool activate_bolt_drain_1(PlayerType *player_ptr);
bool activate_bolt_drain_2(PlayerType *player_ptr);
bool activate_bolt_mana(PlayerType *player_ptr, concptr name);
bool activate_ball_pois_1(PlayerType *player_ptr);
bool activate_ball_cold_1(PlayerType *player_ptr);
bool activate_ball_cold_2(PlayerType *player_ptr);
bool activate_ball_cold_3(PlayerType *player_ptr);
bool activate_ball_fire_1(PlayerType *player_ptr);
bool activate_ball_fire_2(PlayerType *player_ptr, concptr name);
bool activate_ball_fire_3(PlayerType *player_ptr);
bool activate_ball_fire_4(PlayerType *player_ptr);
bool activate_ball_elec_2(PlayerType *player_ptr);
bool activate_ball_elec_3(PlayerType *player_ptr);
bool activate_ball_acid_1(PlayerType *player_ptr);
bool activate_ball_nuke_1(PlayerType *player_ptr);
bool activate_rocket(PlayerType *player_ptr);
bool activate_ball_water(PlayerType *player_ptr, concptr name);
bool activate_ball_lite(PlayerType *player_ptr, concptr name);
bool activate_ball_dark(PlayerType *player_ptr, concptr name);
bool activate_ball_mana(PlayerType *player_ptr, concptr name);
