#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"

struct monster_type;
typedef struct effect_player_type {
    DEPTH rlev; // モンスターのレベル (但し0のモンスターは1になる).
    monster_type *m_ptr;
    char killer[MAX_MONSTER_NAME];
    GAME_TEXT m_name[MAX_NLEN];
    int get_damage;

    MONSTER_IDX who;
    HIT_POINT dam;
    AttributeType attribute;
    BIT_FLAGS flag;
} effect_player_type;
