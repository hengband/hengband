﻿#pragma once

#include "system/angband.h"

class player_type;
void gain_exp_64(player_type *player_ptr, int32_t amount, uint32_t amount_frac);
void gain_exp(player_type *player_ptr, int32_t amount);
bool restore_level(player_type *player_ptr);
void lose_exp(player_type *player_ptr, int32_t amount);
bool drain_exp(player_type *player_ptr, int32_t drain, int32_t slip, int hold_exp_prob);
