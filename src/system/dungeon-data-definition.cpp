#include "system/dungeon-data-definition.h"

DungeonData::DungeonData()
    : room_map(std::vector<std::vector<bool>>(MAX_ROOMS_ROW, std::vector<bool>(MAX_ROOMS_COL)))
    , tunnel_pos(0, 0)
{
    for (auto i = 0; i < CENT_MAX; i++) {
        this->centers.emplace_back(0, 0);
    }

    for (auto i = 0; i < DOOR_MAX; i++) {
        this->doors.emplace_back(0, 0);
    }

    for (auto i = 0; i < WALL_MAX; i++) {
        this->walls.emplace_back(0, 0);
    }

    for (auto i = 0; i < TUNN_MAX; i++) {
        this->tunnels.emplace_back(0, 0);
    }
}
