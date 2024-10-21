#pragma once

#include "system/angband.h"

class PlayerType;
bool has_melee_weapon(PlayerType *player_ptr, int i);
BIT_FLAGS16 empty_hands(PlayerType *player_ptr, bool riding_control);
bool can_two_hands_wielding(PlayerType *player_ptr);
bool heavy_armor(PlayerType *player_ptr);
bool does_equip_has_flag_except_weapon(const BIT_FLAGS &attacker_flags);
