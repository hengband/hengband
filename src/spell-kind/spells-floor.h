#pragma once

#include "system/angband.h"

class PlayerType;
void wiz_lite(PlayerType *player_ptr, bool ninja);
void wiz_dark(PlayerType *player_ptr);
void map_area(PlayerType *player_ptr, POSITION range);
bool destroy_area(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION r, bool in_generate);
