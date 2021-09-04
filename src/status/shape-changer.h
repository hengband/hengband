#pragma once

#include "player/player-race-types.h"
#include "system/angband.h"

struct player_type;
void do_poly_self(player_type *creature_ptr);
void do_poly_wounds(player_type *creature_ptr);
void change_race(player_type *creature_ptr, player_race_type new_race, concptr effect_msg);
