#pragma once

#include "object/tval-types.h"

class PlayerType;
class ItemTester;
void display_inventory(PlayerType *player_ptr, const ItemTester& item_tester);
