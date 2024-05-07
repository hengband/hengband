#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"
#include <optional>

class MonsterEntity;
class EffectPlayerType {
public:
    DEPTH rlev; // モンスターのレベル (但し0のモンスターは1になる).
    MonsterEntity *m_ptr;
    std::string killer;
    GAME_TEXT m_name[MAX_NLEN];
    int get_damage;

    MONSTER_IDX src_idx;
    int dam;
    AttributeType attribute;
    BIT_FLAGS flag;
    EffectPlayerType(MONSTER_IDX src_idx, int dam, AttributeType attribute, BIT_FLAGS flag);
};

struct ProjectResult;
class CapturedMonsterType;
class PlayerType;
using project_func = ProjectResult (*)(
    PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION rad, POSITION y, POSITION x, int dam, AttributeType typ, BIT_FLAGS flag, std::optional<CapturedMonsterType *> cap_mon_ptr);

bool affect_player(MONSTER_IDX src_idx, PlayerType *player_ptr, concptr src_name, int r, POSITION y, POSITION x, int dam, AttributeType typ, BIT_FLAGS flag,
    project_func project);
