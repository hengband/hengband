#pragma once

#include "system/angband.h"

void do_poly_self(player_type *creature_ptr);
void do_poly_wounds(player_type *creature_ptr);
void change_race(player_type *creature_ptr, player_race_type new_race, concptr effect_msg);
bool set_mimic(player_type *creature_ptr, TIME_EFFECT v, MIMIC_RACE_IDX p, bool do_dec);
bool set_shero(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_wraith_form(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tsuyoshi(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
