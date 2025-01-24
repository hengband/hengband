#pragma once

#include <map>
#include <vector>

enum class DoorKind {
    DOOR = 0,
    GLASS_DOOR = 1,
    CURTAIN = 2,
};

enum class TerrainTag;
class Door {
public:
    Door() = default;

    TerrainTag open{};
    TerrainTag broken{};
    TerrainTag closed{};
    std::vector<TerrainTag> locked;
    std::vector<TerrainTag> jammed;
};

extern std::map<DoorKind, Door> feat_door;
