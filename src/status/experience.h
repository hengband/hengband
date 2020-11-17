#pragma once

#include "system/angband.h"

void gain_exp_64(player_type *creature_ptr, s32b amount, u32b amount_frac);
void gain_exp(player_type *creature_ptr, s32b amount);
bool restore_level(player_type *creature_ptr);
void lose_exp(player_type *creature_ptr, s32b amount);
bool drain_exp(player_type *creature_ptr, s32b drain, s32b slip, int hold_exp_prob);
