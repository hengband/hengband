#pragma once

#include "system/angband.h"

class PlayerType;
int32_t turn_real(PlayerType *player_ptr, int32_t hoge);
void prevent_turn_overflow(PlayerType *player_ptr);
