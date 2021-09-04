#pragma once

#include "system/angband.h"

struct player_type;
bool set_tim_levitation(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_ultimate_res(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_nether(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_time(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
