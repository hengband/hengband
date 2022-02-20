#pragma once

class ObjectType;
bool object_is_nameless_weapon_armour(const ObjectType *o_ptr);
bool object_is_not_identified(const ObjectType *o_ptr);
bool object_is_not_identified_weapon_armor(const ObjectType *o_ptr);
bool object_is_not_fully_identified(const ObjectType *o_ptr);
bool object_is_not_fully_identified_weapon_armour(const ObjectType *o_ptr);
