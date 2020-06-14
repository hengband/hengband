#pragma once

#include "system/angband.h"

bool project(player_type *caster_ptr, MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flg, int monspell);
