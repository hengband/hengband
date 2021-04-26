#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool do_cmd_throw(player_type *creature_ptr, int mult, bool boomerang, OBJECT_IDX shuriken);
