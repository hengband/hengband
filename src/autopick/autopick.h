#pragma once

#include "system/angband.h"

class Grid;
class PlayerType;
void autopick_alter_item(PlayerType *player_ptr, INVENTORY_IDX i_idx, bool destroy);
void autopick_delayed_alter(PlayerType *player_ptr);
void autopick_pickup_items(PlayerType *player_ptr, Grid *g_ptr);
