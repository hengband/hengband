#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
void monster_death(player_type *player_ptr, MONSTER_IDX m_idx, bool drop_item);
concptr extract_note_dies(MONRACE_IDX r_idx);
