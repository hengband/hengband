#pragma once

#include <cstdint>

class PlayerType;
bool android_inside_weapon(PlayerType *player_ptr);
void calc_android_exp(PlayerType *player_ptr);
uint64_t android_item_experience(uint64_t value, int64_t level, bool special);
