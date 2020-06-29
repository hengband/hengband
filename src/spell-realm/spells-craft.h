#pragma once

#include "system/angband.h"

bool set_ele_attack(player_type *creature_ptr, u32b attack_type, TIME_EFFECT v);
bool set_ele_immune(player_type *creature_ptr, u32b immune_type, TIME_EFFECT v);
bool choose_ele_attack(player_type *creature_ptr);
bool choose_ele_immune(player_type *creature_ptr, TIME_EFFECT turn);
