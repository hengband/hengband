#pragma once

#include "system/angband.h"

bool project(player_type *caster_ptr, const MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, const HIT_POINT dam, const EFFECT_ID typ, BIT_FLAGS flag,
    const int monspell);
