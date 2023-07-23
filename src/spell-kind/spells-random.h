#pragma once

#include "system/angband.h"

class PlayerType;
void wild_magic(PlayerType *player_ptr, int spell);
void call_chaos(PlayerType *player_ptr);
bool activate_ty_curse(PlayerType *player_ptr, bool stop_ty, int *count);
void cast_wonder(PlayerType *player_ptr, DIRECTION dir);
