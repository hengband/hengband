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
    bool done = false;
    bool will_run = false;
    bool can_pass_wall = false;
    bool can_open_door = false;
    int cost = 0;
    int best = 999;
    bool mon_will_run();
    void sweep_movable_grid(POSITION *yp, POSITION *xp, bool no_flow);
    bool check_movable_grid(POSITION *yp, POSITION *xp, const bool no_flow);
    std::optional<Pos2D> sweep_ranged_attack_grid(const Pos2D &pos_initial);
    std::optional<Pos2DVec> sweep_runnable_away_grid(const Pos2DVec &vec_initial) const;
    std::pair<Pos2DVec, Pos2D> check_hiding_grid(const Pos2DVec &vec_initial, const Pos2D &p_pos);
    Pos2DVec search_room_to_run(const Pos2DVec &vec_initial);
    Pos2DVec search_pet_runnable_grid(const Pos2DVec &vec_initial, bool no_flow);
    Pos2D determine_when_cost(const Pos2D &pos_initial, const Pos2D &m_pos, const bool use_scent);
    bool is_best_cost(const Pos2D &pos, const int now_cost);
};
