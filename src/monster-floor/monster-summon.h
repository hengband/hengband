#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>

enum summon_type : int;
enum class MonraceId : short;
class PlayerType;
tl::optional<MONSTER_IDX> summon_specific(PlayerType *player_ptr, POSITION y1, POSITION x1, DEPTH lev, summon_type type, BIT_FLAGS mode, tl::optional<MONSTER_IDX> summoner_m_idx = tl::nullopt);
tl::optional<MONSTER_IDX> summon_named_creature(PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION oy, POSITION ox, MonraceId r_idx, BIT_FLAGS mode);
