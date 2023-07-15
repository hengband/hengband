#pragma once

#include "floor/floor-base-definitions.h"
#include "system/angband.h"

/*
 * Bounds on some arrays used in the "dun_data_type" structure.
 * These bounds are checked, though usually this is a formality.
 */
#define CENT_MAX 100
#define DOOR_MAX 200
#define WALL_MAX 500
#define TUNN_MAX 900

/*
 * The "size" of a "generation block" in grids
 */
#define BLOCK_HGT 11
#define BLOCK_WID 11

/*
 * Maximum numbers of rooms along each axis (currently 6x6)
 */
#define MAX_ROOMS_ROW (MAX_HGT / BLOCK_HGT)
#define MAX_ROOMS_COL (MAX_WID / BLOCK_WID)

/*
 * Simple structure to hold a map location
 */
struct coord {
    POSITION y;
    POSITION x;
};

/*
 * Structure to hold all "dungeon generation" data
 */
struct dun_data_type {
    /* Array of centers of rooms */
    int cent_n;
    coord cent[CENT_MAX];

    /* Array of possible door locations */
    int door_n;
    coord door[DOOR_MAX];

    /* Array of wall piercing locations */
    int wall_n;
    coord wall[WALL_MAX];

    /* Array of tunnel grids */
    int tunn_n;
    coord tunn[TUNN_MAX];

    /* Number of blocks along each axis */
    int row_rooms;
    int col_rooms;

    /* Array of which blocks are used */
    bool room_map[MAX_ROOMS_ROW][MAX_ROOMS_COL];

    /* Various type of dungeon floors */
    bool destroyed;
    bool empty_level;
    bool cavern;
    int laketype;
    int tunnel_fail_count;

    POSITION tunnel_y;
    POSITION tunnel_x;

    int alloc_object_num;
    int alloc_monster_num;

    concptr *why;
};
