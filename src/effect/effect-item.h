#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"

struct player_type;
bool affect_item(player_type *player_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, AttributeType typ);
