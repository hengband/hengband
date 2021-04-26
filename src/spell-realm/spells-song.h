#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
void check_music(player_type *caster_ptr);
bool set_tim_stealth(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
