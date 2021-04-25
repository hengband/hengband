#pragma once

#include "system/angband.h"

typedef struct floor_type floor_type;
typedef struct player_type player_type;
OBJECT_IDX o_pop(floor_type *floor_ptr);
OBJECT_IDX get_obj_num(player_type *o_ptr, DEPTH level, BIT_FLAGS mode);
