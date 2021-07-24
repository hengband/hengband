#pragma once
#include "system/angband.h"

typedef struct player_type player_type;
void leave_floor(player_type *creature_ptr);
void jump_floor(player_type *creature_ptr, DUNGEON_IDX dun_idx, DEPTH depth);
