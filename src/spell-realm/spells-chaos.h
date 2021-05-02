#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
void call_the_void(player_type *caster_ptr);
bool vanish_dungeon(player_type *caster_ptr);
void cast_meteor(player_type *caster_ptr, HIT_POINT dam, POSITION rad);
