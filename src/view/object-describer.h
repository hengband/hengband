#pragma once

#include "system/angband.h"

void inven_item_charges(player_type *owner_ptr, INVENTORY_IDX item);
void inven_item_describe(player_type *owner_ptr, INVENTORY_IDX item);
void display_koff(player_type *owner_ptr, KIND_OBJECT_IDX k_idx);
