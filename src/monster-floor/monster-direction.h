#pragma once

#include "system/angband.h"
#include <span>

class Direction;
class PlayerType;
bool decide_monster_movement_direction(PlayerType *player_ptr, std::span<Direction> mm, MONSTER_IDX m_idx, bool aware);
