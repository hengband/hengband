#pragma once

struct object_type;
bool object_is_nameless_weapon_armour(const object_type *o_ptr);
bool object_is_not_identified(const object_type *o_ptr);
bool object_is_not_identified_weapon_armor(const object_type *o_ptr);
bool object_is_not_fully_identified(const object_type *o_ptr);
bool object_is_not_fully_identified_weapon_armour(const object_type *o_ptr);
