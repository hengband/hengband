#pragma once

#include "system/angband.h"

typedef struct dun_data_type dun_data_type;
typedef struct player_type player_type;
bool find_space(player_type *player_ptr, dun_data_type *dd_ptr, POSITION *y, POSITION *x, POSITION height, POSITION width);
