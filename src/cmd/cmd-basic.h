#pragma once

#include "system/angband.h"

void forget_travel_flow(floor_type *floor_ptr);
void do_cmd_search(player_type *creature_ptr);
void do_cmd_alter(player_type *creature_ptr);
void do_cmd_suicide(player_type *creature_ptr);
bool do_cmd_throw(player_type *creature_ptr, int mult, bool boomerang, OBJECT_IDX shuriken);

void verify_equip_slot(player_type *owner_ptr, INVENTORY_IDX item);
