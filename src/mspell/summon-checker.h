#pragma once

#include "system/angband.h"

enum class MonsterRaceId : int16_t;
class PlayerType;
bool check_summon_specific(PlayerType *player_ptr, MonsterRaceId summoner_idx, MonsterRaceId r_idx);
