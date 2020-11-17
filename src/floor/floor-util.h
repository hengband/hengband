#pragma once

#include "system/angband.h"

extern floor_type floor_info;

void update_smell(floor_type *floor_ptr, player_type *subject_ptr);
void forget_flow(floor_type *floor_ptr);
void wipe_o_list(floor_type *floor_ptr);
void scatter(player_type *player_ptr, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode);
concptr map_name(player_type *creature_ptr);
