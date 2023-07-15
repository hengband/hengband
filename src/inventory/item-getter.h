#pragma once

#include "system/angband.h"

class PlayerType;
class ItemTester;
bool get_item(PlayerType *player_ptr, OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode, const ItemTester &item_tester);
