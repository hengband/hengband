#pragma once

#include "system/angband.h"

typedef struct monster_type monster_type;
void wr_monster(monster_type *m_ptr);
void wr_lore(MONRACE_IDX r_idx);
