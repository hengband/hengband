#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
bool object_is_orthodox_melee_weapons(const object_type *o_ptr);
bool object_is_broken_weapon(const object_type *o_ptr);
bool object_is_boomerang(const object_type *o_ptr);
bool object_is_mochikae(const object_type *o_ptr);
bool object_is_favorite(player_type *player_ptr, const object_type *o_ptr);
bool object_is_weapon(const object_type *o_ptr);
bool object_is_weapon_ammo(const object_type *o_ptr);
bool object_is_weapon_armour_ammo(const object_type *o_ptr);
bool object_is_melee_weapon(const object_type *o_ptr);
bool object_is_wearable(const object_type *o_ptr);
bool object_is_equipment(const object_type *o_ptr);
bool object_refuse_enchant_weapon(const object_type *o_ptr);
bool object_allow_enchant_weapon(const object_type *o_ptr);
bool object_allow_enchant_melee_weapon(const object_type *o_ptr);
bool object_allow_two_hands_wielding(const object_type *o_ptr);
