#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

typedef struct player_type player_type;
class ItemTester;
bool get_item_floor(player_type *creature_ptr, COMMAND_CODE *cp, concptr pmt, concptr str, BIT_FLAGS mode, const ItemTester& item_tester);
