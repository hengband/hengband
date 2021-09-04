#pragma once

#include "system/angband.h"

struct player_type;
bool cast_wrath_of_the_god(player_type *caster_ptr, HIT_POINT dam, POSITION rad);
bool set_tim_sh_holy(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_eyeeye(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
