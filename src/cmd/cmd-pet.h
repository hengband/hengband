#pragma once

#include "grid/grid.h"

extern int total_friends;

bool do_cmd_riding(player_type *creature_ptr, bool force);
void do_cmd_pet_dismiss(player_type *creature_pt);
void do_cmd_pet(player_type *creature_ptr);
bool player_can_ride_aux(player_type *creature_ptr, grid_type *g_ptr, bool now_riding);
