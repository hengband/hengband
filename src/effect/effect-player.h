#pragma once

bool affect_player(MONSTER_IDX who, player_type *target_ptr, concptr who_name, int r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flag, int monspell);
