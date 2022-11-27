#pragma once

class ItemEntity;
bool object_is_nameless_weapon_armour(const ItemEntity *o_ptr);
bool object_is_not_identified(const ItemEntity *o_ptr);
bool object_is_not_identified_weapon_armor(const ItemEntity *o_ptr);
bool object_is_not_fully_identified(const ItemEntity *o_ptr);
bool object_is_not_fully_identified_weapon_armour(const ItemEntity *o_ptr);
