#pragma once

#include "system/angband.h"

#define NO_TOWN 6
#define SECRET_TOWN 5

enum parse_error_type : int;

/* Wilderness Terrains */
enum wt_type {
	TERRAIN_EDGE = 0, /* Edge of the World */
    TERRAIN_TOWN = 1, /* Town */
	TERRAIN_DEEP_WATER = 2, /* Deep water */
    TERRAIN_SHALLOW_WATER = 3, /* Shallow water */
    TERRAIN_SWAMP = 4, /* Swamp */
    TERRAIN_DIRT = 5, /* Dirt */
    TERRAIN_GRASS = 6, /* Grass */
    TERRAIN_TREES = 7, /* Trees */
    TERRAIN_DESERT = 8, /* Desert */
    TERRAIN_SHALLOW_LAVA = 9, /* Shallow lava */
    TERRAIN_DEEP_LAVA = 10, /* Deep lava */
    TERRAIN_MOUNTAIN = 11, /* Mountain */	
	MAX_WILDERNESS = 12, /* Maximum wilderness index */
};

/*
 * A structure describing a wilderness area with a terrain or a town
 */
typedef struct wilderness_type {
	wt_type terrain;
	int16_t town;
	int road;
	uint32_t seed;
	DEPTH level;
	byte entrance;
} wilderness_type;

extern wilderness_type **wilderness;

typedef struct player_type player_type;
void set_floor_and_wall(DUNGEON_IDX type);
void wilderness_gen(player_type *creature_ptr);
void wilderness_gen_small(player_type *creature_ptr);
errr init_wilderness(void);
void init_wilderness_terrains(void);
void seed_wilderness(void);
parse_error_type parse_line_wilderness(player_type *creature_ptr, char *buf, int xmin, int xmax, int *y, int *x);
bool change_wild_mode(player_type *creature_ptr, bool encount);
