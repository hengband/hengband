#pragma once

#include "system/angband.h"

struct player_type;
void gain_exp_64(player_type *creature_ptr, int32_t amount, uint32_t amount_frac);
void gain_exp(player_type *creature_ptr, int32_t amount);
bool restore_level(player_type *creature_ptr);
void lose_exp(player_type *creature_ptr, int32_t amount);
bool drain_exp(player_type *creature_ptr, int32_t drain, int32_t slip, int hold_exp_prob);
