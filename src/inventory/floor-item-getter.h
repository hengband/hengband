#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

class PlayerType;
class ItemTester;
bool get_item_floor(PlayerType *player_ptr, COMMAND_CODE *cp, concptr pmt, concptr str, BIT_FLAGS mode, const ItemTester &item_tester);
