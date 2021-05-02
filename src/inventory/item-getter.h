#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

typedef struct player_type player_type;
bool get_item(player_type *owner_ptr, OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode, tval_type tval);
