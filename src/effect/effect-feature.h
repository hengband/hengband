#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

class PlayerType;
bool affect_feature(PlayerType *player_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, int dam, AttributeType typ);
