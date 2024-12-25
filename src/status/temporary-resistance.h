#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

class PlayerType;
bool set_tim_levitation(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_ultimate_res(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_timed_resistance(PlayerType *player_ptr, AttributeType attribute, TIME_EFFECT v, bool do_dec);
