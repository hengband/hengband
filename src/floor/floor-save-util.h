#pragma once

#include "system/angband.h"
#include "system/monster-type-definition.h"

#define MAX_SAVED_FLOORS 20 /*!< 保存フロアの最大数 / Maximum number of saved floors. */
#define MAX_PARTY_MON 21 /*!< フロア移動時に先のフロアに連れて行けるペットの最大数 Maximum number of preservable pets */

typedef struct saved_floor_type {
    FLOOR_IDX floor_id; /* No recycle until 65536 IDs are all used */
    int16_t savefile_id; /* ID for savefile (from 0 to MAX_SAVED_FLOOR) */
    DEPTH dun_level;
    int32_t last_visit; /* Time count of last visit. 0 for new floor. */
    uint32_t visit_mark; /* Older has always smaller mark. */
    FLOOR_IDX upper_floor_id; /* a floor connected with level teleportation */
    FLOOR_IDX lower_floor_id; /* a floor connected with level tel. and trap door */
} saved_floor_type;

extern uint32_t saved_floor_file_sign;
extern saved_floor_type saved_floors[MAX_SAVED_FLOORS];
extern FLOOR_IDX max_floor_id;

extern FLOOR_IDX new_floor_id;
extern uint32_t latest_visit_mark;
extern monster_type party_mon[MAX_PARTY_MON];
