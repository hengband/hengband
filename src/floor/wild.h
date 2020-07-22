#pragma once

#include "system/angband.h"
#include "floor/floor-base-definitions.h"

#define NO_TOWN 6
#define SECRET_TOWN 5

/*
 * Wilderness terrains
 */
#define TERRAIN_EDGE             0 /* Edge of the World */
#define TERRAIN_TOWN             1 /* Town */
#define TERRAIN_DEEP_WATER       2 /* Deep water */
#define TERRAIN_SHALLOW_WATER    3 /* Shallow water */
#define TERRAIN_SWAMP            4 /* Swamp */
#define TERRAIN_DIRT             5 /* Dirt */
#define TERRAIN_GRASS            6 /* Grass */
#define TERRAIN_TREES            7 /* Trees */
#define TERRAIN_DESERT           8 /* Desert */
#define TERRAIN_SHALLOW_LAVA     9 /* Shallow lava */
#define TERRAIN_DEEP_LAVA       10 /* Deep lava */
#define TERRAIN_MOUNTAIN        11 /* Mountain */

#define MAX_WILDERNESS          12 /* Maximum wilderness index */

extern void set_floor_and_wall(DUNGEON_IDX type);
extern void wilderness_gen(player_type *creature_ptr);
extern void wilderness_gen_small(player_type *creature_ptr);
extern errr init_wilderness(void);
extern void init_wilderness_terrains(void);
extern void seed_wilderness(void);
extern errr parse_line_wilderness(player_type *creature_ptr, char *buf, int xmin, int xmax, int *y, int *x);
extern bool change_wild_mode(player_type *creature_ptr, bool encount);

/*
 * A structure describing a wilderness area
 * with a terrain or a town
 */
typedef struct wilderness_type {
	int terrain;
	TOWN_IDX town;
	int road;
	u32b seed;
	DEPTH level;
	byte entrance;
} wilderness_type;

extern wilderness_type **wilderness;
