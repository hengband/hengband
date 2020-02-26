#pragma once

#include "angband.h"

void print_tomb(player_type *dead_ptr);
void show_info(player_type *creature_ptr, void(*handle_stuff)(player_type*), void(*update_playtime)(void), void(*displayer_player)(player_type*, int));
