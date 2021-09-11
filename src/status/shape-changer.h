#pragma once

#include "player-info/race-types.h"
#include "system/angband.h"

struct player_type;
void do_poly_self(player_type *player_ptr);
void do_poly_wounds(player_type *player_ptr);
void change_race(player_type *player_ptr, player_race_type new_race, concptr effect_msg);
