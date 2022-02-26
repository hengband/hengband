#pragma once

#include "system/angband.h"

class PlayerType;
class ItemTester;
COMMAND_CODE show_inventory(PlayerType *player_ptr, int target_item, BIT_FLAGS mode, const ItemTester &item_tester);
COMMAND_CODE show_equipment(PlayerType *player_ptr, int target_item, BIT_FLAGS mode, const ItemTester &item_tester);
