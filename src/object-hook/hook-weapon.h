#pragma once

#include "system/angband.h"

bool item_tester_hook_orthodox_melee_weapons(const player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_melee_weapon(const player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_broken_weapon(const player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_boomerang(const player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_mochikae(const player_type *player_ptr, object_type *o_ptr);
bool object_is_favorite(const player_type *player_ptr, object_type *o_ptr);
bool object_is_weapon(const player_type *player_ptr, object_type *o_ptr);
bool object_is_weapon_ammo(object_type *o_ptr);
bool object_is_weapon_armour_ammo(const player_type *player_ptr, object_type *o_ptr);
bool object_is_melee_weapon(object_type *o_ptr);
bool object_is_wearable(object_type *o_ptr);
bool object_is_equipment(object_type *o_ptr);
bool object_refuse_enchant_weapon(object_type *o_ptr);
bool object_allow_enchant_weapon(const player_type *player_ptr, object_type *o_ptr);
bool object_allow_enchant_melee_weapon(const player_type *player_ptr, object_type *o_ptr);
bool object_allow_two_hands_wielding(object_type *o_ptr);
