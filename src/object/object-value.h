#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
struct player_type;
PRICE object_value(const object_type *o_ptr);
PRICE object_value_real(const object_type *o_ptr);
