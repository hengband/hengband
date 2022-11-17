#pragma once

#include "system/angband.h"

class PlayerType;
void inven_item_charges(PlayerType *player_ptr, INVENTORY_IDX item);
void inven_item_describe(PlayerType *player_ptr, INVENTORY_IDX item);
void display_koff(PlayerType *player_ptr, short bi_id);
