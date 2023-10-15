#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

class FloorType;
class PlayerType;
class ItemTester;
bool is_ring_slot(int i);
bool get_tag_floor(FloorType *floor_ptr, COMMAND_CODE *cp, char tag, FLOOR_IDX floor_list[], ITEM_NUMBER floor_num);
bool get_tag(PlayerType *player_ptr, COMMAND_CODE *cp, char tag, BIT_FLAGS mode, const ItemTester &item_tester);
bool get_item_okay(PlayerType *player_ptr, OBJECT_IDX i, const ItemTester &item_tester);
bool get_item_allow(PlayerType *player_ptr, INVENTORY_IDX item);
INVENTORY_IDX label_to_equipment(PlayerType *player_ptr, int c);
INVENTORY_IDX label_to_inventory(PlayerType *player_ptr, int c);
bool verify(PlayerType *player_ptr, concptr prompt, INVENTORY_IDX item);
void prepare_label_string(PlayerType *player_ptr, char *label, BIT_FLAGS mode, const ItemTester &item_tester);
