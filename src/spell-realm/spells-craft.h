﻿#pragma once

#include "system/angband.h"

class player_type;
bool set_ele_attack(player_type *player_ptr, uint32_t attack_type, TIME_EFFECT v);
bool set_ele_immune(player_type *player_ptr, uint32_t immune_type, TIME_EFFECT v);
bool choose_ele_attack(player_type *player_ptr);
bool choose_ele_immune(player_type *player_ptr, TIME_EFFECT turn);
bool pulish_shield(player_type *player_ptr);
