#pragma once

#include "system/angband.h"

/* Mode flags for displaying player flags */
#define DP_WP 1 << 0L
#define DP_CURSE 1 << 1L
#define DP_LITE 1 << 2L

class PlayerType;
void display_player_flag_info_1(PlayerType *player_ptr, void (*display_player_equippy)(PlayerType *, TERM_LEN, TERM_LEN, BIT_FLAGS16));
void display_player_flag_info_2(PlayerType *player_ptr, void (*display_player_equippy)(PlayerType *, TERM_LEN, TERM_LEN, BIT_FLAGS16));
void display_player_flag_info_3(PlayerType *player_ptr, void (*display_player_equippy)(PlayerType *, TERM_LEN, TERM_LEN, BIT_FLAGS16));
