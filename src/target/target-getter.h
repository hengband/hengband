#pragma once

#include "system/angband.h"

class PlayerType;
bool get_aim_dir(PlayerType *player_ptr, DIRECTION *dp);
bool get_direction(PlayerType *player_ptr, DIRECTION *dp, bool allow_under, bool with_steed);
bool get_rep_dir(PlayerType *player_ptr, DIRECTION *dp, bool under);
