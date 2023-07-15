#pragma once

#include "system/angband.h"

#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"

class PlayerType;
void remove_bad_spells(MONSTER_IDX m_idx, PlayerType *player_ptr, EnumClassFlagGroup<MonsterAbilityType> &ability_flags);
