#pragma once

#include "floor/floor-save.h"
#include "system/angband.h"

errr rd_savefile_new(player_type *player_ptr);
bool load_floor(player_type *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode);
