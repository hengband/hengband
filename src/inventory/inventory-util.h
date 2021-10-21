﻿#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

struct floor_type;
class player_type;
class ItemTester;
bool is_ring_slot(int i);
bool get_tag_floor(floor_type *floor_ptr, COMMAND_CODE *cp, char tag, FLOOR_IDX floor_list[], ITEM_NUMBER floor_num);
bool get_tag(player_type *player_ptr, COMMAND_CODE *cp, char tag, BIT_FLAGS mode, const ItemTester& item_tester);
bool get_item_okay(player_type *player_ptr, OBJECT_IDX i, const ItemTester& item_tester);
bool get_item_allow(player_type *player_ptr, INVENTORY_IDX item);
INVENTORY_IDX label_to_equipment(player_type *player_ptr, int c);
INVENTORY_IDX label_to_inventory(player_type *player_ptr, int c);
bool verify(player_type *player_ptr, concptr prompt, INVENTORY_IDX item);
void prepare_label_string(player_type *player_ptr, char *label, BIT_FLAGS mode, const ItemTester& item_tester);
