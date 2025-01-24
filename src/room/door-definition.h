#pragma once

#include <map>
#include <vector>

#define DUN_ROOMS_MAX 40 /*!< 部屋生成処理の基本比率(ダンジョンのサイズに比例する) / Max number rate of rooms */

enum class DoorKind {
    DOOR = 0,
    GLASS_DOOR = 1,
    CURTAIN = 2,
};

enum class LockJam {
    LOCKED,
    JAMMED,
};

/* A structure type for doors */
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
