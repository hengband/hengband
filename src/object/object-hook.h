#pragma once

#include "system/angband.h"
#include "system/object-type-definition.h"

extern bool (*item_tester_hook)(player_type *, object_type *o_ptr);

bool item_tester_hook_recharge(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_eatable(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_activate(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_use(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_quaff(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_readable(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_melee_ammo(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_weapon_except_bow(player_type *player_ptr, object_type *o_ptr);

bool item_tester_learn_spell(player_type *player_ptr, object_type *o_ptr);
bool item_tester_high_level_book(object_type *o_ptr);
bool item_tester_refill_lantern(player_type *player_ptr, object_type *o_ptr);

bool object_is_potion(object_type *o_ptr);
bool object_is_bounty(player_type *player_ptr, object_type *o_ptr);
bool object_is_rare(object_type *o_ptr);
bool object_is_smith(player_type *player_ptr, object_type *o_ptr);
bool object_is_artifact(object_type *o_ptr);
bool object_is_random_artifact(object_type *o_ptr);
bool object_is_nameless(player_type *player_ptr, object_type *o_ptr);
bool object_can_refill_torch(player_type *player_ptr, object_type *o_ptr);
bool can_player_destroy_object(player_type *player_ptr, object_type *o_ptr);
bool object_is_quest_target(player_type *player_ptr, object_type *o_ptr);
bool item_tester_okay(player_type *player_ptr, object_type *o_ptr, tval_type tval);
bool object_is_fixed_artifact(object_type *o_ptr);
bool object_is_ego(object_type *o_ptr);
