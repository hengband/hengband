#pragma once

#include "floor/floor-save.h"
#include "system/angband.h"

enum save_type {
	SAVE_TYPE_CLOSE_GAME = 0,
	SAVE_TYPE_CONTINUE_GAME = 1,
	SAVE_TYPE_DEBUG = 2
};

bool save_player(player_type *player_ptr, save_type type);
