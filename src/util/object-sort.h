#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
bool object_sort_comp(player_type *player_ptr, object_type *o_ptr, int32_t o_value, object_type *j_ptr);
