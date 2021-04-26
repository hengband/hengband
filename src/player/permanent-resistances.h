#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
void player_flags(player_type *creature_ptr, BIT_FLAGS *flags);
void riding_flags(player_type *creature_ptr, BIT_FLAGS *flags, BIT_FLAGS *negative_flags);
