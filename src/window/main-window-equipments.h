﻿#pragma once

#include "object/tval-types.h"

class player_type;
class ItemTester;
void display_inventory(player_type *player_ptr, const ItemTester& item_tester);
