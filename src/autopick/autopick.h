#pragma once

#include "system/angband.h"

typedef struct grid_type grid_type;
typedef struct player_type player_type;
void autopick_alter_item(player_type *player_ptr, INVENTORY_IDX item, bool destroy);
void autopick_delayed_alter(player_type *player_ptr);
void autopick_pickup_items(player_type *player_ptr, grid_type *g_ptr);
