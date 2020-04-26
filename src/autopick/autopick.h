#pragma once

#include "autopick/autopick-util.h"

extern void autopick_alter_item(player_type *player_ptr, INVENTORY_IDX item, bool destroy);
extern void autopick_delayed_alter(player_type *player_ptr);
extern void autopick_pickup_items(player_type *player_ptr, grid_type *g_ptr);
extern void do_cmd_edit_autopick(player_type *player_ptr);
