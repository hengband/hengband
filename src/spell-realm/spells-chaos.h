#pragma once

#include "system/angband.h"

class PlayerType;
void call_the_void(PlayerType *player_ptr);
bool vanish_dungeon(PlayerType *player_ptr);
void cast_meteor(PlayerType *player_ptr, int dam, POSITION rad);
