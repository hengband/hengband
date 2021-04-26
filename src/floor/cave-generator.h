#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool cave_gen(player_type *player_ptr, concptr *why);
