#pragma once

#include "system/angband.h"

#include "object-enchant/tr-flags.h"

struct player_type;
void tim_player_flags(player_type *creature_ptr, TrFlags &flags);
