#pragma once

#include "system/angband.h"

bool item_tester_hook_activate(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_use(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_recharge(player_type *player_ptr, object_type *o_ptr);
bool item_tester_learn_spell(player_type *player_ptr, object_type *o_ptr);
bool item_tester_high_level_book(object_type *o_ptr);
