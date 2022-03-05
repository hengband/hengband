#pragma once

#include "system/angband.h"

enum class MonsterRaceId : int16_t;
struct monster_type;
void wr_monster(monster_type *m_ptr);
void wr_lore(MonsterRaceId r_idx);
