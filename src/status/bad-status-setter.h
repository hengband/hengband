#pragma once

#include "system/angband.h"

struct player_type;
bool set_blind(player_type *creature_ptr, TIME_EFFECT v);
bool set_confused(player_type *creature_ptr, TIME_EFFECT v);
bool set_poisoned(player_type *creature_ptr, TIME_EFFECT v);
bool set_afraid(player_type *creature_ptr, TIME_EFFECT v);
bool set_paralyzed(player_type *creature_ptr, TIME_EFFECT v);
bool set_image(player_type *creature_ptr, TIME_EFFECT v);
bool set_slow(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_stun(player_type *creature_ptr, TIME_EFFECT v);
bool set_cut(player_type *creature_ptr, TIME_EFFECT v);
