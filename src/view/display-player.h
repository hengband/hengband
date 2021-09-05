#pragma once

#include "system/angband.h"

struct player_type;
void display_player(player_type *creature_ptr, int mode);
void display_player_equippy(player_type *player_ptr, TERM_LEN y, TERM_LEN x, BIT_FLAGS16 mode);
