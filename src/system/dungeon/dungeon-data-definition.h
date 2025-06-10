#pragma once

#include "util/point-2d.h"
#include <string>
#include <tl/optional.hpp>
#include <vector>

/*
 * Structure to hold all "dungeon generation" data
 */
class DungeonData {
public:
    DungeonData(const Pos2DVec &dungeon_size);

    size_t cent_n = 0;
    std::vector<Pos2D> centers;

    size_t door_n = 0;
    std::vector<Pos2D> doors;

    size_t wall_n = 0;
    std::vector<Pos2D> walls;

    size_t tunn_n = 0;
    std::vector<Pos2D> tunnels;

    /* Number of blocks along each axis */
    int row_rooms;
    int col_rooms;

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

    tl::optional<std::string> why;
};
