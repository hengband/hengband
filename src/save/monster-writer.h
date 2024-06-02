#pragma once

#include "system/angband.h"

enum class MonsterRaceId : int16_t;
class MonsterEntity;
void wr_monster(const MonsterEntity &monster);
void wr_lore(MonsterRaceId r_idx);
