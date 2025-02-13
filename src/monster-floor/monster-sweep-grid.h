#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <optional>
#include <span>
#include <utility>

class Direction;
class PlayerType;
class MonsterSweepGrid {
public:
    MonsterSweepGrid(PlayerType *player_ptr, MONSTER_IDX m_idx, std::span<Direction> mm);
    virtual ~MonsterSweepGrid() = default;
    PlayerType *player_ptr;
    MONSTER_IDX m_idx;
    std::span<Direction> mm;
    bool get_movable_grid();
};
