#pragma once

#include "system/angband.h"

struct ProjectResult;

struct player_type;
using project_func = ProjectResult (*)(
    player_type *player_ptr, MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flag);

bool affect_player(MONSTER_IDX who, player_type *player_ptr, concptr who_name, int r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flag,
    project_func project);
