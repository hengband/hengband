#pragma once
#include "system/angband.h"

enum class DUNGEON_IDX : int;
typedef struct player_type player_type;
void leave_floor(player_type *creature_ptr);
void jump_floor(player_type *creature_ptr, DUNGEON_IDX dun_idx, DEPTH depth);
