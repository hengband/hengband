#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>

enum class MonraceId : short;
class PlayerType;
tl::optional<MONSTER_IDX> place_monster_one(PlayerType *player_ptr, POSITION y, POSITION x, MonraceId r_idx, BIT_FLAGS mode, tl::optional<MONSTER_IDX> summoner_m_idx = tl::nullopt);
