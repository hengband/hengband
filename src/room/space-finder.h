#pragma once

#include "system/angband.h"

struct dun_data_type;
class PlayerType;
bool find_space(PlayerType *player_ptr, dun_data_type *dd_ptr, POSITION *y, POSITION *x, POSITION height, POSITION width);
