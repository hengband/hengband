#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
void gain_exp_64(player_type *creature_ptr, int amount, uint amount_frac);
void gain_exp(player_type *creature_ptr, int amount);
bool restore_level(player_type *creature_ptr);
void lose_exp(player_type *creature_ptr, int amount);
bool drain_exp(player_type *creature_ptr, int drain, int slip, int hold_exp_prob);
