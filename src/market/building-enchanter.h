#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

typedef struct player_type player_type;
class ItemTester;
bool enchant_item(
    player_type *player_ptr, PRICE cost, HIT_PROB to_hit, HIT_POINT to_dam, ARMOUR_CLASS to_ac, tval_type item_tester_tval, const ItemTester &item_tester);
