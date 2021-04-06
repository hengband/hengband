#pragma once

#include "system/angband.h"

#include "monster-race/race-ability-flags.h"
#include "monster/smart-learn-types.h"
#include "util/flag-group.h"

// Monster Spell Remover.
typedef struct monster_race monster_race;
typedef struct msr_type {
    monster_race *r_ptr;
    FlagGroup<RF_ABILITY> ability_flags;
    FlagGroup<SM> smart;
} msr_type;

msr_type *initialize_msr_type(player_type *target_ptr, msr_type *msr_ptr, MONSTER_IDX m_idx, const FlagGroup<RF_ABILITY> &ability_flags);
bool int_outof(monster_race *r_ptr, PERCENTAGE prob);
