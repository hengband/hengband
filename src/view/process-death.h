#pragma once

#include "angband.h"

void print_tomb(player_type *dead_ptr, void(*read_dead_file)(char*, size_t));
void show_info(player_type *creature_ptr, void(*handle_stuff)(player_type*), errr(*file_character)(player_type*, concptr), void(*update_playtime)(void), void(*displayer_player)(player_type*, int));
