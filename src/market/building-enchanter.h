#pragma once

#include "system/angband.h"

struct player_type;
class ItemTester;
bool enchant_item(player_type *player_ptr, PRICE cost, HIT_PROB to_hit, HIT_POINT to_dam, ARMOUR_CLASS to_ac, const ItemTester &item_tester);
