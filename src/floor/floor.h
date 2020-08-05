#pragma once

#include "system/angband.h"

extern floor_type floor_info;

/*
 * Convert a "location" (Y,X) into a "grid" (G)
 */
#define GRID(Y,X) \
	(256 * (Y) + (X))

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

bool pattern_tile(floor_type *floor_ptr, POSITION y, POSITION x);
void update_smell(floor_type *floor_ptr, player_type *subject_ptr);
void add_door(player_type *player_ptr, POSITION x, POSITION y);
void place_secret_door(player_type *player_ptr, POSITION y, POSITION x, int type);
void place_locked_door(player_type *player_ptr, POSITION y, POSITION x);
void forget_flow(floor_type *floor_ptr);
void place_random_stairs(player_type *player_ptr, POSITION y, POSITION x);
bool los(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
int get_max_range(player_type *creature_ptr);
bool projectable(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
void vault_monsters(player_type *player_ptr, POSITION y1, POSITION x1, int num);
bool cave_valid_bold(floor_type *floor_ptr, POSITION y, POSITION x);
void cave_set_feat(player_type *player_ptr, POSITION y, POSITION x, FEAT_IDX feat);
void place_random_door(player_type *player_ptr, POSITION y, POSITION x, bool room);
void place_closed_door(player_type *player_ptr, POSITION y, POSITION x, int type);
void wipe_o_list(floor_type *floor_ptr);
bool get_is_floor(floor_type *floor_ptr, POSITION x, POSITION y);
FEAT_IDX conv_dungeon_feat(floor_type *floor_ptr, FEAT_IDX newfeat);
void vault_objects(player_type *player_ptr, POSITION y, POSITION x, int num);
int project_path(player_type *player_ptr, u16b *gp, POSITION range, POSITION y1, POSITION x1, POSITION y2, POSITION x2, BIT_FLAGS flg);
void set_floor(player_type *player_ptr, POSITION x, POSITION y);
void place_object(player_type *owner_ptr, POSITION y, POSITION x, BIT_FLAGS mode);
void place_gold(player_type *player_ptr, POSITION y, POSITION x);
void delete_monster(player_type *player_ptr, POSITION y, POSITION x);
void compact_objects(player_type *owner_ptr, int size);
void vault_traps(player_type *player_ptr, POSITION y, POSITION x, POSITION yd, POSITION xd, int num);
void scatter(player_type *player_ptr, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode);
