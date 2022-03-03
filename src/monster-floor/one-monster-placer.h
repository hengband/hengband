#pragma once

#include "system/angband.h"

enum class MonsterRaceId;
class PlayerType;
bool place_monster_one(PlayerType *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MonsterRaceId r_idx, BIT_FLAGS mode);
