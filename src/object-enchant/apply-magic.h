#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
void apply_magic(player_type *owner_type, object_type *o_ptr, DEPTH lev, BIT_FLAGS mode);
