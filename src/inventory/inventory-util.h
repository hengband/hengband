#pragma once

#include "object/tval-types.h"
#include "system/angband.h"
#include <tl/optional.hpp>
#include <vector>

class FloorType;
class PlayerType;
class ItemTester;
bool is_ring_slot(int i);
tl::optional<short> get_tag_floor(const FloorType &floor, char tag, const std::vector<short> &floor_item_index);
bool get_tag(PlayerType *player_ptr, COMMAND_CODE *cp, char tag, BIT_FLAGS mode, const ItemTester &item_tester);
bool get_item_okay(PlayerType *player_ptr, OBJECT_IDX i, const ItemTester &item_tester);
bool get_item_allow(PlayerType *player_ptr, INVENTORY_IDX i_idx);
INVENTORY_IDX label_to_equipment(PlayerType *player_ptr, int c);
INVENTORY_IDX label_to_inventory(PlayerType *player_ptr, int c);
bool verify(PlayerType *player_ptr, concptr prompt, INVENTORY_IDX i_idx);
std::string prepare_label_string(PlayerType *player_ptr, BIT_FLAGS mode, const ItemTester &item_tester);
