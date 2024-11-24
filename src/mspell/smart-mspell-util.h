#pragma once

#include "monster-race/race-ability-flags.h"
#include "monster/smart-learn-types.h"
#include "util/flag-group.h"

// Monster Spell Remover.
class MonraceDefinition;
class PlayerType;
struct msr_type {
    msr_type(PlayerType *player_ptr, short m_idx, const EnumClassFlagGroup<MonsterAbilityType> &ability_flags);
    MonraceDefinition *r_ptr;
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    EnumClassFlagGroup<MonsterSmartLearnType> smart_flags{};
};

bool int_outof(MonraceDefinition *r_ptr, int prob);
