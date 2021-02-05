#pragma once

#include "floor/floor-save.h"
#include "system/angband.h"

typedef enum save_type {
	SAVE_TYPE_NORMAL = 0,
	SAVE_TYPE_DEBUG = 1
}save_type;

bool save_player(player_type *player_ptr, save_type type);
