#pragma once

#include "system/angband.h"
#include "spell/spell-types.h"

struct player_type;
bool affect_monster(player_type *player_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flag, bool see_s_msg);
