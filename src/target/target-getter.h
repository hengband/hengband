#pragma once

#include "system/angband.h"

class PlayerType;
bool get_aim_dir(PlayerType *player_ptr, int *dp);
bool get_direction(PlayerType *player_ptr, int *dp);
bool get_rep_dir(PlayerType *player_ptr, DIRECTION *dp, bool under);
