#pragma once

#include "system/angband.h"

class FloorType;
class PlayerType;
OBJECT_IDX o_pop(FloorType *floor_ptr);
OBJECT_IDX get_obj_num(PlayerType *player_ptr, DEPTH level, BIT_FLAGS mode);
