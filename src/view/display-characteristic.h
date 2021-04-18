#pragma once

#include "system/angband.h"

/* Mode flags for displaying player flags */
#define DP_CURSE   0x01
#define DP_IMM     0x02
#define DP_WP      0x08

void display_player_flag_info_1(player_type *creature_ptr, void(*display_player_equippy)(player_type*, TERM_LEN, TERM_LEN, BIT_FLAGS16));
void display_player_flag_info_2(player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16));
void display_player_flag_info_3(player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16));
