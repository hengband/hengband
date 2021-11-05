#pragma once

#include "system/angband.h"

class PlayerType;
void monster_death(PlayerType *player_ptr, MONSTER_IDX m_idx, bool drop_item);
concptr extract_note_dies(MONRACE_IDX r_idx);
