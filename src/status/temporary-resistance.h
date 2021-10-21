#pragma once

#include "system/angband.h"

class player_type;
bool set_tim_levitation(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_ultimate_res(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_nether(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_res_time(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
