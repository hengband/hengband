#pragma once

#include "system/angband.h"

class PlayerType;
bool set_ele_attack(PlayerType *player_ptr, uint32_t attack_type, TIME_EFFECT v);
bool set_ele_immune(PlayerType *player_ptr, uint32_t immune_type, TIME_EFFECT v);
bool choose_ele_attack(PlayerType *player_ptr);
bool choose_ele_immune(PlayerType *player_ptr, TIME_EFFECT turn);
bool pulish_shield(PlayerType *player_ptr);
