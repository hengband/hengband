#pragma once

#include "system/angband.h"

class PlayerType;
bool pattern_effect(PlayerType *player_ptr);
bool pattern_seq(PlayerType *player_ptr, POSITION c_y, POSITION c_x, POSITION n_y, POSITION n_x);
void pattern_teleport(PlayerType *player_ptr);
