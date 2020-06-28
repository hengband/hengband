#pragma once

#include "system/angband.h"
#include "system/object-type-definition.h"

extern bool (*item_tester_hook)(player_type *, object_type *o_ptr);

bool item_tester_hook_melee_ammo(player_type *player_ptr, object_type *o_ptr);
bool object_is_smith(player_type *player_ptr, object_type *o_ptr);
bool item_tester_okay(player_type *player_ptr, object_type *o_ptr, tval_type tval);
