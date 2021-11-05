#pragma once

#include "system/angband.h"

class PlayerType;
bool affect_monster(PlayerType *player_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flag, bool see_s_msg);
