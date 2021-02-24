#pragma once

#include "system/angband.h"

bool item_tester_hook_eatable(const player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_quaff(const player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_readable(const player_type *player_ptr, object_type *o_ptr);
bool item_tester_refill_lantern(const player_type *player_ptr, object_type *o_ptr);
bool object_can_refill_torch(const player_type *player_ptr, object_type *o_ptr);
bool can_player_destroy_object(player_type *player_ptr, object_type *o_ptr);
bool object_is_potion(object_type *o_ptr);
