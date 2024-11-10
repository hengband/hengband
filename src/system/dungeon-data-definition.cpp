#include "system/dungeon-data-definition.h"

DungeonData::DungeonData()
    : room_map(std::vector<std::vector<bool>>(MAX_ROOMS_ROW, std::vector<bool>(MAX_ROOMS_COL)))
    , tunnel_pos(0, 0)
{
    for (auto i = 0; i < CENT_MAX; i++) {
        this->cent.emplace_back(0, 0);
    }

    for (auto i = 0; i < DOOR_MAX; i++) {
        this->door.emplace_back(0, 0);
    }

    for (auto i = 0; i < WALL_MAX; i++) {
        this->wall.emplace_back(0, 0);
    }

    for (auto i = 0; i < TUNN_MAX; i++) {
        this->tunn.emplace_back(0, 0);
    }
}
