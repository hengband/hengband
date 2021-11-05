﻿#pragma once

#include "system/angband.h"

struct ProjectResult;

class PlayerType;
using project_func = ProjectResult (*)(
    PlayerType *player_ptr, MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flag);

bool affect_player(MONSTER_IDX who, PlayerType *player_ptr, concptr who_name, int r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flag,
    project_func project);
