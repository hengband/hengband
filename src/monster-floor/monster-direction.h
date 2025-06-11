#pragma once

#include "monster-floor/monster-movement-direction-list.h"
#include "system/angband.h"
#include <tl/optional.hpp>

class Direction;
class PlayerType;
tl::optional<MonsterMovementDirectionList> decide_monster_movement_direction(PlayerType *player_ptr, MONSTER_IDX m_idx, bool aware);
