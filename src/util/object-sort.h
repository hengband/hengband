#pragma once

#include "system/angband.h"

struct object_type;
class PlayerType;
bool object_sort_comp(PlayerType *player_ptr, object_type *o_ptr, int32_t o_value, object_type *j_ptr);
