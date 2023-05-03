#pragma once

#include "system/angband.h"
#include <vector>

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
struct wilderness_type {
    wt_type terrain;
    int16_t town;
    int road;
    uint32_t seed;
    DEPTH level;
    byte entrance;
};

extern std::vector<std::vector<wilderness_type>> wilderness;

class PlayerType;
void set_floor_and_wall(DUNGEON_IDX type);
void wilderness_gen(PlayerType *player_ptr);
void wilderness_gen_small(PlayerType *player_ptr);
void init_wilderness_terrains();
void init_wilderness_encounter();
void seed_wilderness(void);
parse_error_type parse_line_wilderness(PlayerType *player_ptr, char *buf, int xmin, int xmax, int *y, int *x);
bool change_wild_mode(PlayerType *player_ptr, bool encount);
