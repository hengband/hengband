#pragma once

#include "object/tval-types.h"
#include "system/angband.h"

bool get_item_floor(player_type *creature_ptr, COMMAND_CODE *cp, concptr pmt, concptr str, BIT_FLAGS mode, tval_type tval);
