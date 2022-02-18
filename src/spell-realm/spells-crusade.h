#pragma once

#include "system/angband.h"

class PlayerType;
bool cast_wrath_of_the_god(PlayerType *player_ptr, int dam, POSITION rad);
bool set_tim_sh_holy(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_eyeeye(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
