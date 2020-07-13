#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

bool can_get_item(player_type *owner_ptr, tval_type tval);
void py_pickup_floor(player_type *creature_ptr, bool pickup);
