#pragma once

#include "object-enchant/item-feeling.h"

class ObjectType;
class PlayerType;
void sense_inventory1(PlayerType *player_ptr);
void sense_inventory2(PlayerType *player_ptr);
item_feel_type pseudo_value_check_heavy(ObjectType *o_ptr);
item_feel_type pseudo_value_check_light(ObjectType *o_ptr);
