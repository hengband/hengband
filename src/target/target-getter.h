#pragma once

#include "floor/geometry.h"

class PlayerType;
bool get_aim_dir(PlayerType *player_ptr, int *dp);
Direction get_direction(PlayerType *player_ptr);
bool get_rep_dir(PlayerType *player_ptr, int *dp, bool under = false);
