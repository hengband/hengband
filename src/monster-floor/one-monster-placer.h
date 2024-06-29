#pragma once

#include "system/angband.h"
#include <optional>

enum class MonsterRaceId : int16_t;
class PlayerType;
std::optional<MONSTER_IDX> place_monster_one(PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION y, POSITION x, MonsterRaceId r_idx, BIT_FLAGS mode, std::optional<MONSTER_IDX> summoner_m_idx = std::nullopt);
