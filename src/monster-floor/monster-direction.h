#pragma once

#include "monster-floor/monster-movement-direction-list.h"
#include "system/angband.h"
#include <optional>

class Direction;
class PlayerType;
std::optional<MonsterMovementDirectionList> decide_monster_movement_direction(PlayerType *player_ptr, MONSTER_IDX m_idx, bool aware);
