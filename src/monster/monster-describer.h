#pragma once

#include "system/angband.h"

typedef struct monster_type monster_type;
typedef struct player_type player_type;
void monster_desc(player_type *player_ptr, char *desc, monster_type *m_ptr, BIT_FLAGS mode);
void message_pain(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam);
