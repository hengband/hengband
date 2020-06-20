#pragma once

#include "system/angband.h"

void get_project_point(player_type *target_ptr, POSITION sy, POSITION sx, POSITION *ty, POSITION *tx, BIT_FLAGS flg);
bool monst_spell_monst(player_type *target_ptr, MONSTER_IDX m_idx);
