﻿#pragma once

#include "system/angband.h"

class player_type;
bool cast_wrath_of_the_god(player_type *player_ptr, HIT_POINT dam, POSITION rad);
bool set_tim_sh_holy(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_tim_eyeeye(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
