#pragma once

#include "system/angband.h"

void remove_bad_spells(MONSTER_IDX m_idx, player_type *target_ptr, FlagGroup<RF_ABILITY>& ability_flags);
