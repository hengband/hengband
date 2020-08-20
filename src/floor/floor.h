#pragma once

#include "system/angband.h"

extern floor_type floor_info;

/*
 * Convert a "grid" (G) into a "location" (Y)
 */
#define GRID_Y(G) \
	((int)((G) / 256U))

/*
 * Convert a "grid" (G) into a "location" (X)
 */
#define GRID_X(G) \
	((int)((G) % 256U))

void update_smell(floor_type *floor_ptr, player_type *subject_ptr);
void forget_flow(floor_type *floor_ptr);
void place_random_stairs(player_type *player_ptr, POSITION y, POSITION x);
int get_max_range(player_type *creature_ptr);
bool projectable(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
bool cave_valid_bold(floor_type *floor_ptr, POSITION y, POSITION x);
void wipe_o_list(floor_type *floor_ptr);
bool get_is_floor(floor_type *floor_ptr, POSITION x, POSITION y);
FEAT_IDX conv_dungeon_feat(floor_type *floor_ptr, FEAT_IDX newfeat);
void set_floor(player_type *player_ptr, POSITION x, POSITION y);
void compact_objects(player_type *owner_ptr, int size);
void scatter(player_type *player_ptr, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode);
