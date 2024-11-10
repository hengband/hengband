#include "system/dungeon-data-definition.h"

DungeonData::DungeonData()
    : room_map(std::vector<std::vector<bool>>(MAX_ROOMS_ROW, std::vector<bool>(MAX_ROOMS_COL)))
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
}
