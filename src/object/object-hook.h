#pragma once

#include "system/angband.h"
#include "system/object-type-definition.h"

extern bool (*item_tester_hook)(player_type *, object_type *o_ptr);

bool item_tester_hook_recharge(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_eatable(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_activate(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_wear(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_use(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_quaff(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_readable(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_melee_ammo(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_weapon_except_bow(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_cursed(player_type *player_ptr, object_type *o_ptr);

bool item_tester_learn_spell(player_type *player_ptr, object_type *o_ptr);
bool item_tester_high_level_book(object_type *o_ptr);
bool item_tester_refill_lantern(player_type *player_ptr, object_type *o_ptr);

bool object_is_potion(object_type *o_ptr);
bool object_is_bounty(player_type *player_ptr, object_type *o_ptr);
bool object_is_favorite(player_type *player_ptr, object_type *o_ptr);
bool object_is_rare(object_type *o_ptr);
bool object_is_weapon(player_type *player_ptr, object_type *o_ptr);
bool object_is_weapon_ammo(object_type *o_ptr);
bool object_is_armour(player_type *player_ptr, object_type *o_ptr);
bool object_is_weapon_armour_ammo(player_type *player_ptr, object_type *o_ptr);
bool object_is_melee_weapon(object_type *o_ptr);
bool object_is_wearable(object_type *o_ptr);
bool object_is_equipment(object_type *o_ptr);
bool object_refuse_enchant_weapon(object_type *o_ptr);
bool object_allow_enchant_weapon(player_type *player_ptr, object_type *o_ptr);
bool object_allow_enchant_melee_weapon(player_type *player_ptr, object_type *o_ptr);
bool object_is_smith(player_type *player_ptr, object_type *o_ptr);
bool object_is_artifact(object_type *o_ptr);
bool object_is_random_artifact(object_type *o_ptr);
bool object_is_nameless(player_type *player_ptr, object_type *o_ptr);
bool object_allow_two_hands_wielding(object_type *o_ptr);
bool object_can_refill_torch(player_type *player_ptr, object_type *o_ptr);
bool can_player_destroy_object(player_type *player_ptr, object_type *o_ptr);
bool object_is_quest_target(player_type *player_ptr, object_type *o_ptr);

#define OBJECT_IS_VALID(T) ((T)->k_idx != 0)

#define OBJECT_IS_HELD_MONSTER(T) ((T)->held_m_idx != 0)

/*
 * Artifacts use the "name1" field
 */
#define object_is_fixed_artifact(T) ((T)->name1 ? TRUE : FALSE)

/*
 * Ego-Items use the "name2" field
 */
#define object_is_ego(T) ((T)->name2 ? TRUE : FALSE)

/*
 * Broken items.
 */
#define object_is_broken(T) ((T)->ident & (IDENT_BROKEN))

/*
 * Cursed items.
 */
#define object_is_cursed(T) ((T)->curse_flags)

bool item_tester_okay(player_type *player_ptr, object_type *o_ptr, tval_type tval);
