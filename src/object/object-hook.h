#pragma once

#include "system/angband.h"
#include "system/object-type-definition.h"

extern bool (*item_tester_hook)(player_type *, object_type *o_ptr);

bool item_tester_hook_melee_ammo(player_type *player_ptr, object_type *o_ptr);
bool object_is_bounty(player_type *player_ptr, object_type *o_ptr);
bool object_is_rare(object_type *o_ptr);
bool object_is_smith(player_type *player_ptr, object_type *o_ptr);
bool object_is_artifact(object_type *o_ptr);
bool object_is_random_artifact(object_type *o_ptr);
bool object_is_nameless(player_type *player_ptr, object_type *o_ptr);
bool object_is_quest_target(player_type *player_ptr, object_type *o_ptr);
bool item_tester_okay(player_type *player_ptr, object_type *o_ptr, tval_type tval);
bool object_is_fixed_artifact(object_type *o_ptr);
bool object_is_ego(object_type *o_ptr);
