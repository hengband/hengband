#pragma once

#include <map>

#define DUN_ROOMS_MAX 40 /*!< 部屋生成処理の基本比率(ダンジョンのサイズに比例する) / Max number rate of rooms */

/* Maximum locked/jammed doors */
#define MAX_LJ_DOORS 8

enum door_kind_type : int {
    DOOR_DEFAULT = -1,
    DOOR_DOOR = 0,
    DOOR_GLASS_DOOR = 1,
    DOOR_CURTAIN = 2,
};

/* A structure type for doors */
struct door_type {
    short open;
    short broken;
    short closed;
    short locked[MAX_LJ_DOORS];
    short num_locked;
    short jammed[MAX_LJ_DOORS];
    short num_jammed;
};

extern std::map<door_kind_type, door_type> feat_door;
