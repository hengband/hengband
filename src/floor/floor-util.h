#pragma once

#include "system/angband.h"

struct floor_type;
extern floor_type floor_info;

struct player_type;
void update_smell(floor_type *floor_ptr, player_type *player_ptr);
void forget_flow(floor_type *floor_ptr);
void wipe_o_list(floor_type *floor_ptr);
void scatter(player_type *player_ptr, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode);
concptr map_name(player_type *player_ptr);
