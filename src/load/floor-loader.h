#pragma once

#include "system/angband.h"

class PlayerType;
struct saved_floor_type;
errr rd_saved_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr);
bool load_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode);
