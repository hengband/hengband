#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

class PlayerType;
bool affect_item(PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION r, POSITION y, POSITION x, int dam, AttributeType typ);
