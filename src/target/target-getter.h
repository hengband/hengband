#pragma once

#include "floor/geometry.h"

class PlayerType;
Direction get_aim_dir(PlayerType *player_ptr, bool enable_repeat = true);
Direction get_direction(PlayerType *player_ptr);
Direction get_rep_dir(PlayerType *player_ptr, bool under = false);
