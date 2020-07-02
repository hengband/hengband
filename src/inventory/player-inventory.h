#pragma once

#include "object/tval-types.h"
#include "system/angband.h"
#include "system/object-type-definition.h"

void display_inventory(player_type *creature_ptr, tval_type tval);
COMMAND_CODE show_inventory(player_type *owner_ptr, int target_item, BIT_FLAGS mode, tval_type tval);
COMMAND_CODE show_equipment(player_type *owner_ptr, int target_item, BIT_FLAGS mode, tval_type tval);
bool can_get_item(player_type *owner_ptr, tval_type tval);
void py_pickup_floor(player_type *creature_ptr, bool pickup);
