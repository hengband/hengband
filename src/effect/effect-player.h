#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"

struct monster_type;
class EffectPlayerType {
public:
    DEPTH rlev; // モンスターのレベル (但し0のモンスターは1になる).
    monster_type *m_ptr;
    char killer[MAX_MONSTER_NAME];
    GAME_TEXT m_name[MAX_NLEN];
    int get_damage;

    MONSTER_IDX who;
    HIT_POINT dam;
    AttributeType attribute;
    BIT_FLAGS flag;
    EffectPlayerType(MONSTER_IDX who, HIT_POINT dam, AttributeType attribute, BIT_FLAGS flag);

};


struct ProjectResult;

class PlayerType;
using project_func = ProjectResult (*)(
    PlayerType *player_ptr, MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, HIT_POINT dam, AttributeType typ, BIT_FLAGS flag);

bool affect_player(MONSTER_IDX who, PlayerType *player_ptr, concptr who_name, int r, POSITION y, POSITION x, HIT_POINT dam, AttributeType typ, BIT_FLAGS flag,
    project_func project);
