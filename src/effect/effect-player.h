#pragma once

#include "system/angband.h"

typedef bool (*project_func)(
    player_type *caster_ptr, MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, HIT_POINT dam, spell_type typ, BIT_FLAGS flag, int monspell);

bool affect_player(MONSTER_IDX who, player_type *target_ptr, concptr who_name, int r, POSITION y, POSITION x, HIT_POINT dam, spell_type typ, BIT_FLAGS flag,
    int monspell, project_func project);
