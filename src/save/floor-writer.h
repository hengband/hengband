#pragma once

#include "system/angband.h"

/* Flags for save/load temporary saved floor file */
#define SLF_SECOND 0x0001 /* Called from another save/load function */
#define SLF_NO_KILL 0x0002 /* Don't kill temporary files */

class PlayerType;
struct saved_floor_type;
void wr_saved_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr);
bool wr_dungeon(PlayerType *player_ptr);
bool save_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode);
