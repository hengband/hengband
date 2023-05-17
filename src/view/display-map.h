#pragma once

#include "system/angband.h"

extern byte display_autopick;

class PlayerType;
void map_info(PlayerType *player_ptr, POSITION y, POSITION x, TERM_COLOR *ap, char *cp, TERM_COLOR *tap, char *tcp);
