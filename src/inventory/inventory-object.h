#pragma once

#include "system/angband.h"

class ObjectType;
class PlayerType;
void vary_item(PlayerType *player_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
void inven_item_increase(PlayerType *player_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
void inven_item_optimize(PlayerType *player_ptr, INVENTORY_IDX item);
void drop_from_inventory(PlayerType *player_ptr, INVENTORY_IDX item, ITEM_NUMBER amt);
void combine_pack(PlayerType *player_ptr);
void reorder_pack(PlayerType *player_ptr);
int16_t store_item_to_inventory(PlayerType *player_ptr, ObjectType *o_ptr);
bool check_store_item_to_inventory(PlayerType *player_ptr, const ObjectType *o_ptr);
INVENTORY_IDX inven_takeoff(PlayerType *player_ptr, INVENTORY_IDX item, ITEM_NUMBER amt);
