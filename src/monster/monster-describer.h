#pragma once

#include "system/angband.h"
#include "system/monster-type-definition.h"

void monster_desc(player_type *player_ptr, char *desc, monster_type *m_ptr, BIT_FLAGS mode);
void message_pain(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam);
