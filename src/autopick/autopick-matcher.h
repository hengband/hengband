#pragma once

#include "system/angband.h"
#include "autopick/autopick-util.h"

bool is_autopick_match(player_type *player_ptr, object_type *o_ptr, autopick_type *entry, concptr o_name);
