#pragma once

#include "system/angband.h"

class PlayerType;
void gain_exp_64(PlayerType *player_ptr, int32_t amount, uint32_t amount_frac);
void gain_exp(PlayerType *player_ptr, int32_t amount);
bool restore_level(PlayerType *player_ptr);
void lose_exp(PlayerType *player_ptr, int32_t amount);
bool drain_exp(PlayerType *player_ptr, int32_t drain, int32_t slip, int hold_exp_prob);
