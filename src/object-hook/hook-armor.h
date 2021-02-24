#pragma once

#include "system/angband.h"

bool item_tester_hook_wear(const player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_cursed(const player_type *player_ptr, object_type *o_ptr);
bool object_is_armour(const player_type *player_ptr, object_type *o_ptr);
