#pragma once

#include "object-enchant/item-feeling.h"

class ItemEntity;
class PlayerType;
void sense_inventory1(PlayerType *player_ptr);
void sense_inventory2(PlayerType *player_ptr);
item_feel_type pseudo_value_check_heavy(ItemEntity *o_ptr);
item_feel_type pseudo_value_check_light(ItemEntity *o_ptr);
