#pragma once

#include "monster-floor/monster-movement-direction-list.h"
#include "system/angband.h"
#include "util/point-2d.h"
#include <tl/optional.hpp>
#include <utility>

class Direction;
class PlayerType;
class MonsterSweepGrid {
public:
    MonsterSweepGrid(PlayerType *player_ptr, MONSTER_IDX m_idx);
    PlayerType *player_ptr;
    MONSTER_IDX m_idx;
    tl::optional<MonsterMovementDirectionList> get_movable_grid();
};
