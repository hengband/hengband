#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
bool item_tester_hook_orthodox_melee_weapons(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_melee_weapon(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_broken_weapon(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_boomerang(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_mochikae(player_type *player_ptr, object_type *o_ptr);
bool object_is_favorite(player_type *player_ptr, object_type *o_ptr);
bool object_is_weapon(player_type *player_ptr, object_type *o_ptr);
bool object_is_weapon_ammo(object_type *o_ptr);
bool object_is_weapon_armour_ammo(player_type *player_ptr, object_type *o_ptr);
bool object_is_melee_weapon(object_type *o_ptr);
bool object_is_wearable(object_type *o_ptr);
bool object_is_equipment(object_type *o_ptr);
bool object_refuse_enchant_weapon(object_type *o_ptr);
bool object_allow_enchant_weapon(player_type *player_ptr, object_type *o_ptr);
bool object_allow_enchant_melee_weapon(player_type *player_ptr, object_type *o_ptr);
bool object_allow_two_hands_wielding(object_type *o_ptr);
