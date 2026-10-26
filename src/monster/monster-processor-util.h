/*!
 * @brief monster-processのための構造体群
 * @date 2020/03/07
 * @author Hourier
 */

#pragma once

#include "monster-floor/monster-movement-direction-list.h"
#include "system/angband.h"
#include "util/point-2d.h"
#include <tl/optional.hpp>

struct turn_flags {
    bool see_m;
    bool aware;
    bool is_riding_mon;
    bool do_turn;
    bool do_move;
    bool do_view;
    bool do_take;
    bool must_alter_to_move;

    bool did_open_door;
    bool did_bash_door;
    bool did_take_item;
    bool did_kill_item;
    bool did_move_body;
    bool did_pass_wall;
    bool did_kill_wall;
};

struct coordinate_candidate {
    Pos2D pos = { 0, 0 };
    int gdis = 0;
};

turn_flags *init_turn_flags(bool is_riding, turn_flags *turn_flags_ptr);

class Direction;
MonsterMovementDirectionList get_enemy_approch_direction(MONSTER_IDX m_idx, const Pos2DVec &vec);
tl::optional<MonsterMovementDirectionList> get_moves_val(MONSTER_IDX m_idx, const Pos2DVec &vec);
