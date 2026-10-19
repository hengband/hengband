#pragma once

#include "object-enchant/item-feeling.h"
#include "system/angband.h"

class ItemEntity;
class PlayerType;
void apply_item_feeling(PlayerType *player_ptr, INVENTORY_IDX i_idx, item_feel_type feel);
void sense_inventory1(PlayerType *player_ptr);
void sense_inventory2(PlayerType *player_ptr);
item_feel_type pseudo_value_check_heavy(const ItemEntity *o_ptr);
item_feel_type pseudo_value_check_light(const ItemEntity *o_ptr);
