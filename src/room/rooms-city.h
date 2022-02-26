#pragma once

#include "system/angband.h"

/* Minimum & maximum town size */
#define MIN_TOWN_WID ((MAX_WID / 3) / 2)
#define MIN_TOWN_HGT ((MAX_HGT / 3) / 2)
#define MAX_TOWN_WID ((MAX_WID / 3) * 2 / 3)
#define MAX_TOWN_HGT ((MAX_HGT / 3) * 2 / 3)

/* Struct for build underground buildings */
struct ugbldg_type {
    POSITION y0, x0; /* North-west corner (relative) */
    POSITION y1, x1; /* South-east corner (relative) */
};

struct dun_data_type;
class PlayerType;
bool build_type16(PlayerType *player_ptr, dun_data_type *dd_ptr);
