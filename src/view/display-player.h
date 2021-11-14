#pragma once

#include "system/angband.h"

class PlayerType;
void display_player(PlayerType *player_ptr, int mode);
void display_player_equippy(PlayerType *player_ptr, TERM_LEN y, TERM_LEN x, BIT_FLAGS16 mode);
