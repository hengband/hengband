#pragma once

#include "object/tval-types.h"

typedef struct player_type player_type;
class ItemTester;
void display_inventory(player_type *creature_ptr, const ItemTester& item_tester);
