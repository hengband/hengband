#pragma once

#include "system/angband.h"

struct player_type;
bool set_resist_magic(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
