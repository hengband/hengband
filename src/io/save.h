#pragma once

#include "floor/floor-save.h"

/*
 * Flags for save/load temporary saved floor file
 */
#define SLF_SECOND     	 0x0001  /* Called from another save/load function */
#define SLF_NO_KILL      0x0002  /* Don't kill temporary files */

bool save_player(player_type *player_ptr);
bool load_player(player_type *player_ptr);
bool save_floor(player_type *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode);
