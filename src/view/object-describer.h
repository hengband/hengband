#pragma once

#include "system/angband.h"

struct player_type;
void inven_item_charges(player_type *player_ptr, INVENTORY_IDX item);
void inven_item_describe(player_type *player_ptr, INVENTORY_IDX item);
void display_koff(player_type *player_ptr, KIND_OBJECT_IDX k_idx);
