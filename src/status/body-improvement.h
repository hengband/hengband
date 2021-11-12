#pragma once

#include "system/angband.h"

class PlayerType;
bool set_protevil(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_invuln(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_regen(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_reflect(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_pass_wall(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
