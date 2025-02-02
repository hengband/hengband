#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <optional>
#include <utility>

class PlayerType;
class MonsterSweepGrid {
public:
    MonsterSweepGrid(PlayerType *player_ptr, MONSTER_IDX m_idx, DIRECTION *mm);
    virtual ~MonsterSweepGrid() = default;
    PlayerType *player_ptr;
    MONSTER_IDX m_idx;
    DIRECTION *mm;
    bool get_movable_grid();

private:
    std::optional<Pos2DVec> sweep_runnable_away_grid(const Pos2DVec &vec_initial) const;
    Pos2DVec search_pet_runnable_grid(const Pos2DVec &vec_initial, bool will_run, bool no_flow);
};
