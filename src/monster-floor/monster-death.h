#pragma once

#include "system/angband.h"

struct player_type;
void monster_death(player_type *player_ptr, MONSTER_IDX m_idx, bool drop_item);
concptr extract_note_dies(MONRACE_IDX r_idx);
