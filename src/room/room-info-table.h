#pragma once

#include "room/room-types.h"
#include "system/angband.h"

/* Room type information */
struct room_info_type {
    int16_t prob[ROOM_TYPE_MAX]; /* Allocation information. */
    byte min_level; /* Minimum level on which room can appear. */
};

extern room_info_type room_info_normal[ROOM_TYPE_MAX];
extern RoomType room_build_order[ROOM_TYPE_MAX];
