#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class Grid;
class PlayerType;
struct turn_flags;
void exe_monster_attack_to_player(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, const Pos2D &pos);
bool process_monster_attack_to_monster(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, Grid *g_ptr, bool can_cross);
