#pragma once

#include "system/angband.h"

struct player_type;
struct saved_floor_type;
errr rd_saved_floor(player_type *player_ptr, saved_floor_type *sf_ptr);
bool load_floor(player_type *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode);
