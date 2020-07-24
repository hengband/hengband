#pragma once

#include "system/angband.h"

#define DUN_ROOMS_MAX 40 /*!< 部屋生成処理の最大試行数 / Number of rooms to attempt (was 50) */

/* Maximum locked/jammed doors */
#define MAX_LJ_DOORS 8

#define MAX_DOOR_TYPES 3

/* A structure type for doors */
typedef struct door_type {
    FEAT_IDX open;
    FEAT_IDX broken;
    FEAT_IDX closed;
    FEAT_IDX locked[MAX_LJ_DOORS];
    FEAT_IDX num_locked;
    FEAT_IDX jammed[MAX_LJ_DOORS];
    FEAT_IDX num_jammed;
} door_type;

extern door_type feat_door[MAX_DOOR_TYPES];

void build_lake(player_type *player_ptr, int type);
void build_cavern(player_type *player_ptr);
void build_maze_vault(player_type *player_ptr, POSITION x0, POSITION y0, POSITION xsize, POSITION ysize, bool is_vault);
bool find_space(player_type *player_ptr, POSITION *y, POSITION *x, POSITION height, POSITION width);
void build_small_room(player_type *player_ptr, POSITION x0, POSITION y0);
void add_outer_wall(player_type *player_ptr, POSITION x, POSITION y, int light, POSITION x1, POSITION y1, POSITION x2, POSITION y2);
POSITION dist2(POSITION x1, POSITION y1, POSITION x2, POSITION y2, POSITION h1, POSITION h2, POSITION h3, POSITION h4);
void build_recursive_room(player_type *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int power);
void build_room(player_type *player_ptr, POSITION x1, POSITION x2, POSITION y1, POSITION y2);
void r_visit(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int node, DIRECTION dir, int *visited);
