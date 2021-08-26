#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
class ItemTester;
COMMAND_CODE show_inventory(player_type *owner_ptr, int target_item, BIT_FLAGS mode, const ItemTester& item_tester);
COMMAND_CODE show_equipment(player_type *owner_ptr, int target_item, BIT_FLAGS mode, const ItemTester& item_tester);
