#pragma once

#include "system/angband.h"

#define DUN_ROOMS_MAX 40 /*!< 部屋生成処理の基本比率(ダンジョンのサイズに比例する) / Max number rate of rooms */

/* Maximum locked/jammed doors */
#define MAX_LJ_DOORS 8

#define MAX_DOOR_TYPES 3

/* A structure type for doors */
typedef struct door_type {
    FEAT_IDX open;
    FEAT_IDX broken;
    FEAT_IDX closed;
    FEAT_IDX locked[MAX_LJ_DOORS];
    FEAT_IDX num_locked;
    FEAT_IDX jammed[MAX_LJ_DOORS];
    FEAT_IDX num_jammed;
} door_type;

extern door_type feat_door[MAX_DOOR_TYPES];
