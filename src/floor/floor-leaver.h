#pragma once
#include "system/angband.h"

struct player_type;
void leave_floor(player_type *player_ptr);
void jump_floor(player_type *player_ptr, DUNGEON_IDX dun_idx, DEPTH depth);
