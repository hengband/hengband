#pragma once

#include "system/angband.h"

bool item_tester_hook_orthodox_melee_weapons(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_melee_weapon(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_broken_weapon(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_boomerang(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_mochikae(player_type *player_ptr, object_type *o_ptr);
