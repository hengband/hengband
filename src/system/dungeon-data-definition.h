#pragma once

#include "floor/floor-base-definitions.h"
#include "util/point-2d.h"
#include <optional>
#include <string>
#include <vector>

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
 * Structure to hold all "dungeon generation" data
 */
class DungeonData {
public:
    DungeonData();

    size_t cent_n = 0;
    std::vector<Pos2D> centers;

    size_t door_n = 0;
    std::vector<Pos2D> doors;

    size_t wall_n = 0;
    std::vector<Pos2D> walls;

    size_t tunn_n = 0;
    std::vector<Pos2D> tunnels;

    /* Number of blocks along each axis */
    int row_rooms = 0;
    int col_rooms = 0;

    /* Array of which blocks are used */
    std::vector<std::vector<bool>> room_map;

    /* Various type of dungeon floors */
    bool destroyed = false;
    bool empty_level = false;
    bool cavern = false;
    int laketype = 0;
    int tunnel_fail_count = 0;

    Pos2D tunnel_pos;

    int alloc_object_num = 0;
    int alloc_monster_num = 0;

    std::optional<std::string> why;
};
