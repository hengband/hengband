#pragma once

#include "system/angband.h"

class FloorType;
OBJECT_IDX o_pop(FloorType *floor_ptr);
OBJECT_IDX get_obj_index(const FloorType *floor_ptr, DEPTH level, BIT_FLAGS mode);
