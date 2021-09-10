#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

struct player_type;
class ItemTester;
bool can_get_item(player_type *player_ptr, const ItemTester& item_tester);
void py_pickup_floor(player_type *player_ptr, bool pickup);
void describe_pickup_item(player_type *player_ptr, OBJECT_IDX o_idx);
void carry(player_type *player_ptr, bool pickup);
