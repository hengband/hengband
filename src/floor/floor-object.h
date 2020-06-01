#pragma once

#include "system/angband.h"

bool make_object(player_type *owner_ptr, object_type *j_ptr, BIT_FLAGS mode);
bool make_gold(floor_type *floor_ptr, object_type *j_ptr);
