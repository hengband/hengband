#pragma once

#include "system/angband.h"

struct dun_data_type;
struct player_type;
bool find_space(player_type *player_ptr, dun_data_type *dd_ptr, POSITION *y, POSITION *x, POSITION height, POSITION width);
