#pragma once

#include "system/angband.h"

extern floor_type floor_info;

/*
 * Grid based version of "creature_bold()"
 */
#define player_grid(C, G) \
	((G) == &(C)->current_floor_ptr->grid_array[(C)->y][(C)->x])


#define cave_have_flag_grid(C,INDEX) \
	(have_flag(f_info[(C)->feat].flags, (INDEX)))


/*
 * Determine if a "legal" grid is a "clean" floor grid
 * Determine if terrain-change spells are allowed in a grid.
 *
 * Line 1 -- forbid non-floors
 * Line 2 -- forbid object terrains
 * Line 3 -- forbid normal objects
 */
#define cave_clean_bold(F,Y,X) \
	(cave_have_flag_bold((F), (Y), (X), FF_FLOOR) && \
	 !((F)->grid_array[Y][X].info & CAVE_OBJECT) && \
	  ((F)->grid_array[Y][X].o_idx == 0))


/*
 * Determine if an object can be dropped on a "legal" grid
 *
 * Line 1 -- forbid non-drops
 * Line 2 -- forbid object terrains
 */
#define cave_drop_bold(F,Y,X) \
	(cave_have_flag_bold((F), (Y), (X), FF_DROP) && \
	 !((F)->grid_array[Y][X].info & CAVE_OBJECT))


/*
 * Determine if a "legal" grid is an "naked" floor grid
 *
 * Line 1 -- forbid non-clean gird
 * Line 2 -- forbid monsters
 * Line 3 -- forbid the player
 */
#define cave_naked_bold(C,F,Y,X) \
	(cave_clean_bold(F,Y,X) && \
	 !((F)->grid_array[Y][X].m_idx) && \
	 !player_bold(C,Y,X))


/*
 * Determine if a "legal" grid is "permanent"
 *
 * Line 1 -- permanent flag
 */
#define cave_perma_bold(F,Y,X) \
	(cave_have_flag_bold((F), (Y), (X), FF_PERMANENT))


/*
 * Grid based version of "cave_perma_bold()"
 */
#define cave_perma_grid(C) \
	(cave_have_flag_grid((C), FF_PERMANENT))


/*
 * Determine if a "legal" grid is within "los" of the player
 *
 * Note the use of comparison to zero to force a "boolean" result
 */
#define player_has_los_grid(C) \
    (((C)->info & (CAVE_VIEW)) != 0)

/*
 * Determine if a "feature" is "permanent wall"
 */
#define permanent_wall(F) \
	(have_flag((F)->flags, FF_WALL) && \
	 have_flag((F)->flags, FF_PERMANENT))

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
bool projectable(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
void vault_monsters(player_type *player_ptr, POSITION y1, POSITION x1, int num);
bool cave_valid_bold(floor_type *floor_ptr, POSITION y, POSITION x);
void cave_set_feat(player_type *player_ptr, POSITION y, POSITION x, FEAT_IDX feat);
void place_random_door(player_type *player_ptr, POSITION y, POSITION x, bool room);
void place_closed_door(player_type *player_ptr, POSITION y, POSITION x, int type);
void wipe_o_list(floor_type *floor_ptr);
void vault_trap_aux(player_type *player_ptr, POSITION y, POSITION x, POSITION yd, POSITION xd);
bool get_is_floor(floor_type *floor_ptr, POSITION x, POSITION y);
void try_door(player_type *player_ptr, POSITION y, POSITION x);
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
