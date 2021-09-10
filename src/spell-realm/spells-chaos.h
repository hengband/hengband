#pragma once

#include "system/angband.h"

struct player_type;
void call_the_void(player_type *player_ptr);
bool vanish_dungeon(player_type *player_ptr);
void cast_meteor(player_type *player_ptr, HIT_POINT dam, POSITION rad);
