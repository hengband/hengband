﻿#pragma once

#include "system/angband.h"

struct monster_type;
typedef struct effect_PlayerType {
    DEPTH rlev; // モンスターのレベル (但し0のモンスターは1になる).
    monster_type *m_ptr;
    char killer[MAX_MONSTER_NAME];
    GAME_TEXT m_name[MAX_NLEN];
    int get_damage;

    MONSTER_IDX who;
    HIT_POINT dam;
    EFFECT_ID effect_type;
    BIT_FLAGS flag;
} effect_PlayerType;
