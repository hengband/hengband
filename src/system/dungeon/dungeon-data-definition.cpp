#include "system/dungeon/dungeon-data-definition.h"
#include "floor/floor-base-definitions.h"

DungeonData::DungeonData(const Pos2DVec &dungeon_size)
    : row_rooms(dungeon_size.y / BLOCK_HGT)
    , col_rooms(dungeon_size.x / BLOCK_WID)
    , tunnel_pos(0, 0)
{
    constexpr auto max_centers = 100;
    for (auto i = 0; i < max_centers; i++) {
        this->centers.emplace_back(0, 0);
    }

    constexpr auto max_doors = 200;
    for (auto i = 0; i < max_doors; i++) {
        this->doors.emplace_back(0, 0);
    }

    constexpr auto max_walls = 500;
    for (auto i = 0; i < max_walls; i++) {
        this->walls.emplace_back(0, 0);
    }

    constexpr auto max_tunnels = 900;
    for (auto i = 0; i < max_tunnels; i++) {
        this->tunnels.emplace_back(0, 0);
    }

    constexpr auto max_rooms_row = MAX_HGT / BLOCK_HGT;
    constexpr auto max_rooms_col = MAX_WID / BLOCK_WID;
    this->room_map = std::vector<std::vector<bool>>(max_rooms_row, std::vector<bool>(max_rooms_col));
}
