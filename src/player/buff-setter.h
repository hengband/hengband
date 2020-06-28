#pragma once

#include "system/angband.h"

bool set_fast(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_shield(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_magicdef(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_blessed(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_hero(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
