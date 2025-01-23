#pragma once

#include <map>

#define DUN_ROOMS_MAX 40 /*!< 部屋生成処理の基本比率(ダンジョンのサイズに比例する) / Max number rate of rooms */

/* Maximum locked/jammed doors */
#define MAX_LJ_DOORS 8

enum class DoorKind {
    DEFAULT = -1,
    DOOR = 0,
    GLASS_DOOR = 1,
    CURTAIN = 2,
};

/* A structure type for doors */
class Door {
public:
    Door() = default;

    short open = 0;
    short broken = 0;
    short closed = 0;
    short locked[MAX_LJ_DOORS]{};
    short num_locked = 0;
    short jammed[MAX_LJ_DOORS]{};
    short num_jammed = 0;
};

extern std::map<DoorKind, Door> feat_door;
