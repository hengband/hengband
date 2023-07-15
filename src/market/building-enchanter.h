#pragma once

#include "system/angband.h"

class PlayerType;
class ItemTester;
bool enchant_item(PlayerType *player_ptr, PRICE cost, HIT_PROB to_hit, int to_dam, ARMOUR_CLASS to_ac, const ItemTester &item_tester);
