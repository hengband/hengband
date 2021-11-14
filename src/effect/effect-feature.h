#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"

class PlayerType;
bool affect_feature(PlayerType *player_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, AttributeType typ);
