#pragma once

#include "system/angband.h"

#include "object-enchant/tr-flags.h"

struct player_type;
void player_flags(player_type *player_ptr, TrFlags &flags);
void riding_flags(player_type *player_ptr, TrFlags &flags, TrFlags &negative_flags);
