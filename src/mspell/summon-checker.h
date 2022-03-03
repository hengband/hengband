#pragma once

#include "system/angband.h"

enum class MonsterRaceId;
class PlayerType;
bool check_summon_specific(PlayerType *player_ptr, MonsterRaceId summoner_idx, MonsterRaceId r_idx);
