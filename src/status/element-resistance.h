#pragma once

#include "system/angband.h"

class PlayerType;
bool set_oppose_acid(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_oppose_elec(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_oppose_fire(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_oppose_cold(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_oppose_pois(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool is_oppose_acid(PlayerType *player_ptr);
bool is_oppose_elec(PlayerType *player_ptr);
bool is_oppose_fire(PlayerType *player_ptr);
bool is_oppose_cold(PlayerType *player_ptr);
bool is_oppose_pois(PlayerType *player_ptr);
