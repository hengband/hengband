#pragma once

#include "system/angband.h"

struct floor_type;
struct player_type;
class MonsterSweepGrid {
public:
    MonsterSweepGrid(player_type *target_ptr, MONSTER_IDX m_idx, DIRECTION *mm);
    MonsterSweepGrid() = delete;
    virtual ~MonsterSweepGrid() = default;
    player_type *target_ptr;
    MONSTER_IDX m_idx;
    DIRECTION *mm;
    bool get_movable_grid();

private:
    bool done = false;
    bool will_run = false;
    bool can_pass_wall = false;
    bool mon_will_run();
    void sweep_movable_grid(POSITION *yp, POSITION *xp, bool no_flow);
    bool check_movable_grid(POSITION *yp, POSITION *xp, const bool no_flow);
    bool sweep_ranged_attack_grid(POSITION *yp, POSITION *xp);
    bool sweep_runnable_away_grid(POSITION *yp, POSITION *xp);
    void check_hiding_grid(POSITION *y, POSITION *x, POSITION *y2, POSITION *x2);
    void search_room_to_run(POSITION *y, POSITION *x);
    void search_pet_runnable_grid(POSITION *y, POSITION *x, bool no_flow);
};
