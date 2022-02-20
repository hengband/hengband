#pragma once

#include "system/angband.h"

struct monster_type;
class PlayerType;
void monster_desc(PlayerType *player_ptr, char *desc, monster_type *m_ptr, BIT_FLAGS mode);
void message_pain(PlayerType *player_ptr, MONSTER_IDX m_idx, int dam);
