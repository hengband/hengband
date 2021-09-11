#pragma once

#include "system/angband.h"

/* Mode flags for displaying player flags */
#define DP_WP 1<<0L
#define DP_CURSE 1<<1L
#define DP_LITE 1<<2L

struct player_type;
void display_player_flag_info_1(player_type *player_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16));
void display_player_flag_info_2(player_type *player_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16));
void display_player_flag_info_3(player_type *player_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16));
