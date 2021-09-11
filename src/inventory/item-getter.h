#pragma once

#include "system/angband.h"

struct player_type;
class ItemTester;
bool get_item(player_type *player_ptr, OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode, const ItemTester& item_tester);
