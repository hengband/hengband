#pragma once

#include "system/angband.h"

class PlayerType;
bool set_tim_esp(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_invis(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_infra(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
