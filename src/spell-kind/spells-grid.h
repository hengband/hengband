#pragma once

#include "system/angband.h"

class PlayerType;
bool create_rune_protection_one(PlayerType *player_ptr);
bool create_rune_explosion(PlayerType *player_ptr, POSITION y, POSITION x);
void stair_creation(PlayerType *player_ptr);
