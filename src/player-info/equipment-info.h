#pragma once

#include "system/angband.h"

struct player_type;
bool has_melee_weapon(player_type *player_ptr, int i);
BIT_FLAGS16 empty_hands(player_type *player_ptr, bool riding_control);
bool can_two_hands_wielding(player_type *player_ptr);
bool heavy_armor(player_type *player_ptr);
