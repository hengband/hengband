﻿#pragma once

#include "room/room-types.h"
#include "system/angband.h"

/* Room type information */
typedef struct room_info_type {
    short prob[ROOM_T_MAX]; /* Allocation information. */
    byte min_level; /* Minimum level on which room can appear. */
} room_info_type;

extern room_info_type room_info_normal[ROOM_T_MAX];
extern byte room_build_order[ROOM_T_MAX];
