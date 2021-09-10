#pragma once

#include "system/angband.h"

struct floor_type;
struct player_type;
OBJECT_IDX o_pop(floor_type *floor_ptr);
OBJECT_IDX get_obj_num(player_type *player_ptr, DEPTH level, BIT_FLAGS mode);
