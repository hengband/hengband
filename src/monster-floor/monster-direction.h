#pragma once

#include "system/angband.h"

struct player_type;
bool get_enemy_dir(player_type *player_ptr, MONSTER_IDX m_idx, int *mm);
bool decide_monster_movement_direction(player_type *player_ptr, DIRECTION *mm, MONSTER_IDX m_idx, bool aware);
