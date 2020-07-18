#pragma once

#include "system/angband.h"

typedef enum feature_flag_type feature_flag_type;
bool in_bounds(floor_type *floor_ptr, POSITION y, POSITION x);
bool in_bounds2(floor_type *floor_ptr, POSITION y, POSITION x);
bool cave_have_flag_bold(floor_type *floor_ptr, POSITION y, POSITION x, feature_flag_type f_idx);
