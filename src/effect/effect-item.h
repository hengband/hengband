#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool affect_item(player_type *caster_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ);
