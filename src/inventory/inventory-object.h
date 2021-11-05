#pragma once

#include "system/angband.h"

struct object_type;
class PlayerType;
void vary_item(PlayerType *player_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
void inven_item_increase(PlayerType *player_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
void inven_item_optimize(PlayerType *player_ptr, INVENTORY_IDX item);
void drop_from_inventory(PlayerType *player_ptr, INVENTORY_IDX item, ITEM_NUMBER amt);
void combine_pack(PlayerType *player_ptr);
void reorder_pack(PlayerType *player_ptr);
int16_t store_item_to_inventory(PlayerType *player_ptr, object_type *o_ptr);
bool check_store_item_to_inventory(PlayerType *player_ptr, const object_type *o_ptr);
INVENTORY_IDX inven_takeoff(PlayerType *player_ptr, INVENTORY_IDX item, ITEM_NUMBER amt);
