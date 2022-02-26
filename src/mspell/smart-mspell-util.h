#pragma once

#include "monster-race/race-ability-flags.h"
#include "monster/smart-learn-types.h"
#include "system/angband.h"
#include "util/flag-group.h"

// Monster Spell Remover.
struct monster_race;
struct msr_type {
    monster_race *r_ptr;
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    EnumClassFlagGroup<MonsterSmartLearnType> smart;
};

class PlayerType;
msr_type *initialize_msr_type(PlayerType *player_ptr, msr_type *msr_ptr, MONSTER_IDX m_idx, const EnumClassFlagGroup<MonsterAbilityType> &ability_flags);
bool int_outof(monster_race *r_ptr, PERCENTAGE prob);
