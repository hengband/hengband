#pragma once

#include "effect/effect-processor.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-data.h"
#include "mspell/mspell-result.h"
#include "system/angband.h"
#include <functional>

class PlayerType;
class AbstractMSpellAttack {
public:
    virtual ~AbstractMSpellAttack() = default;
    void operator=(const AbstractMSpellAttack &) = delete;
    AbstractMSpellAttack(const AbstractMSpellAttack &) = delete;
    AbstractMSpellAttack &operator=(AbstractMSpellAttack &&) = default;
    AbstractMSpellAttack(AbstractMSpellAttack &&) = default;
    MonsterSpellResult shoot(POSITION y, POSITION x);

protected:
    AbstractMSpellAttack(PlayerType *player_ptr, MONSTER_IDX m_idx, MonsterAbilityType ability, MSpellData data, int target_type, std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire);
    AbstractMSpellAttack(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, MonsterAbilityType ability, MSpellData data, int target_type, std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire);
    PlayerType *player_ptr;
    MONSTER_IDX m_idx;
    MONSTER_IDX t_idx;
    MonsterAbilityType ability;
    MSpellData data;
    int target_type;
    std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire;
};
