#pragma once

#include "angband.h"

// todo コールバックは暫定。後で設計を組み直す.
typedef bool(*get_moves_pf)(player_type*, MONSTER_IDX, DIRECTION*);

bool get_enemy_dir(player_type *target_ptr, MONSTER_IDX m_idx, int *mm);
bool decide_monster_movement_direction(player_type *target_ptr, DIRECTION *mm, MONSTER_IDX m_idx, bool aware, get_moves_pf get_movable_grid);
