#pragma once

#include "system/angband.h"

struct player_type;
bool get_aim_dir(player_type *creature_ptr, DIRECTION *dp);
bool get_direction(player_type *creature_ptr, DIRECTION *dp, bool allow_under, bool with_steed);
bool get_rep_dir(player_type *creature_ptr, DIRECTION *dp, bool under);
