#pragma once

#include "angband.h"
#include "files.h" // 消すかどうか検討中.

void display_player_flag_info(player_type *creature_ptr, void(*display_player_equippy)(player_type*, TERM_LEN, TERM_LEN, BIT_FLAGS16));
void display_player_other_flag_info(player_type *creature_ptr, void(*display_player_equippy)(player_type*, TERM_LEN, TERM_LEN, BIT_FLAGS16));
