#pragma once

#include "system/angband.h"

void vary_item(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
void inven_item_increase(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
void inven_item_optimize(player_type *owner_ptr, INVENTORY_IDX item);
