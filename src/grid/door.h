#pragma once

#include "system/angband.h"

struct player_type;
void add_door(player_type *player_ptr, POSITION x, POSITION y);
void place_secret_door(player_type *player_ptr, POSITION y, POSITION x, int type);
void place_locked_door(player_type *player_ptr, POSITION y, POSITION x);
void place_random_door(player_type *player_ptr, POSITION y, POSITION x, bool room);
void place_closed_door(player_type *player_ptr, POSITION y, POSITION x, int type);
