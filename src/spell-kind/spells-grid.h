#pragma once

#include "system/angband.h"

struct player_type;
bool create_rune_protection_one(player_type *player_ptr);
bool create_rune_explosion(player_type *player_ptr, POSITION y, POSITION x);
void stair_creation(player_type *player_ptr);
