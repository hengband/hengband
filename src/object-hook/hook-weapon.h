#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
bool object_is_orthodox_melee_weapons(object_type *o_ptr);
bool object_is_broken_weapon(object_type *o_ptr);
bool object_is_boomerang(object_type *o_ptr);
bool object_is_mochikae(object_type *o_ptr);
bool object_is_favorite(player_type *player_ptr, object_type *o_ptr);
bool object_is_weapon(object_type *o_ptr);
bool object_is_weapon_ammo(object_type *o_ptr);
bool object_is_weapon_armour_ammo(object_type *o_ptr);
bool object_is_melee_weapon(object_type *o_ptr);
bool object_is_wearable(object_type *o_ptr);
bool object_is_equipment(object_type *o_ptr);
bool object_refuse_enchant_weapon(object_type *o_ptr);
bool object_allow_enchant_weapon(object_type *o_ptr);
bool object_allow_enchant_melee_weapon(object_type *o_ptr);
bool object_allow_two_hands_wielding(object_type *o_ptr);
