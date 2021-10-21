﻿#pragma once

#include "system/angband.h"

struct object_type;;
class player_type;
void vary_item(player_type *player_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
void inven_item_increase(player_type *player_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
void inven_item_optimize(player_type *player_ptr, INVENTORY_IDX item);
void drop_from_inventory(player_type *player_ptr, INVENTORY_IDX item, ITEM_NUMBER amt);
void combine_pack(player_type *player_ptr);
void reorder_pack(player_type *player_ptr);
int16_t store_item_to_inventory(player_type *player_ptr, object_type *o_ptr);
bool check_store_item_to_inventory(player_type *player_ptr, const object_type *o_ptr);
INVENTORY_IDX inven_takeoff(player_type *player_ptr, INVENTORY_IDX item, ITEM_NUMBER amt);
