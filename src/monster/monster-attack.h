#pragma once

#include "angband.h"
#include "monster/monster-util.h"

void exe_monster_attack_to_player(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION ny, POSITION nx);
bool process_monster_attack_to_monster(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, grid_type *g_ptr, bool can_cross);
