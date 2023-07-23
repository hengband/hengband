#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

class PlayerType;
class ItemTester;
bool can_get_item(PlayerType *player_ptr, const ItemTester &item_tester);
void py_pickup_floor(PlayerType *player_ptr, bool pickup);
void describe_pickup_item(PlayerType *player_ptr, OBJECT_IDX o_idx);
void carry(PlayerType *player_ptr, bool pickup);
