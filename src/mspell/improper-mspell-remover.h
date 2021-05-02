#pragma once

#include "system/angband.h"

#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"

typedef struct player_type player_type;
void remove_bad_spells(MONSTER_IDX m_idx, player_type *target_ptr, EnumClassFlagGroup<RF_ABILITY> &ability_flags);
