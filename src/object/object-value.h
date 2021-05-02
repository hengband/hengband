#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
PRICE object_value(player_type *player_ptr, object_type *o_ptr);
PRICE object_value_real(player_type *player_ptr, object_type *o_ptr);
