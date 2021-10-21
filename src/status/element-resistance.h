﻿#pragma once

#include "system/angband.h"

class player_type;
bool set_oppose_acid(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_oppose_elec(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_oppose_fire(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_oppose_cold(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_oppose_pois(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool is_oppose_acid(player_type *player_ptr);
bool is_oppose_elec(player_type *player_ptr);
bool is_oppose_fire(player_type *player_ptr);
bool is_oppose_cold(player_type *player_ptr);
bool is_oppose_pois(player_type *player_ptr);
