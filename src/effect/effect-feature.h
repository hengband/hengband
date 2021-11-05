#pragma once

#include "system/angband.h"

class PlayerType;
bool affect_feature(PlayerType *player_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ);
