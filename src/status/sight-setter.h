#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool set_tim_esp(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_invis(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_infra(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
