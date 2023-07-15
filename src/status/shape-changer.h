#pragma once

#include "player-info/race-types.h"
#include "system/angband.h"

class PlayerType;
void do_poly_self(PlayerType *player_ptr);
void do_poly_wounds(PlayerType *player_ptr);
void change_race(PlayerType *player_ptr, PlayerRaceType new_race, concptr effect_msg);
