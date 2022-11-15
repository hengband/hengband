#pragma once

#include "system/angband.h"

enum class MonsterRaceId : int16_t;
class MonsterEntity;
void wr_monster(MonsterEntity *m_ptr);
void wr_lore(MonsterRaceId r_idx);
