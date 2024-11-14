#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"
#include <optional>

class MonsterEntity;
class FallOffHorseEffect;
class FloorType;
class EffectPlayerType {
public:
    EffectPlayerType(const FloorType &floor, short src_idx, int dam, AttributeType attribute, BIT_FLAGS flag);
    DEPTH rlev; // モンスターのレベル (但し0のモンスターは1になる).
    MonsterEntity *m_ptr;
    short src_idx;
    const MonsterEntity *src_ptr;
    std::string killer;
    std::string m_name;
    int get_damage;

    int dam;
    AttributeType attribute;
    BIT_FLAGS flag;

    bool is_monster() const;
};

struct ProjectResult;
class CapturedMonsterType;
class PlayerType;
using project_func = ProjectResult (*)(
    PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION rad, POSITION y, POSITION x, int dam, AttributeType typ, BIT_FLAGS flag, std::optional<CapturedMonsterType *> cap_mon_ptr);

bool affect_player(MONSTER_IDX src_idx, PlayerType *player_ptr, concptr src_name, int r, POSITION y, POSITION x, int dam, AttributeType typ, BIT_FLAGS flag,
    FallOffHorseEffect &fall_off_horse_effect, project_func project);
