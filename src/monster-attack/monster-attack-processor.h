#pragma once

#include "system/angband.h"

struct grid_type;
;
class PlayerType;
struct turn_flags;
void exe_monster_attack_to_player(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION ny, POSITION nx);
bool process_monster_attack_to_monster(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, grid_type *g_ptr, bool can_cross);
