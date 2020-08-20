#pragma once

#include "system/angband.h"

extern floor_type floor_info;

void update_smell(floor_type *floor_ptr, player_type *subject_ptr);
void forget_flow(floor_type *floor_ptr);
void place_random_stairs(player_type *player_ptr, POSITION y, POSITION x);
bool cave_valid_bold(floor_type *floor_ptr, POSITION y, POSITION x);
void wipe_o_list(floor_type *floor_ptr);
bool get_is_floor(floor_type *floor_ptr, POSITION x, POSITION y);
void set_floor(player_type *player_ptr, POSITION x, POSITION y);
void compact_objects(player_type *owner_ptr, int size);
void scatter(player_type *player_ptr, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode);
