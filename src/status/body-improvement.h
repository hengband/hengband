#pragma once

#include "system/angband.h"

struct player_type;
bool set_protevil(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_invuln(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_regen(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_reflect(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_pass_wall(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
