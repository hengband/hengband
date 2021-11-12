#pragma once

#include "system/angband.h"

struct floor_type;
class PlayerType;
OBJECT_IDX o_pop(floor_type *floor_ptr);
OBJECT_IDX get_obj_num(PlayerType *player_ptr, DEPTH level, BIT_FLAGS mode);
