#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
struct player_type;
int32_t flag_cost(const object_type *o_ptr, int plusses);
