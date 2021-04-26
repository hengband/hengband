#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool get_enemy_dir(player_type *target_ptr, MONSTER_IDX m_idx, int *mm);
bool decide_monster_movement_direction(player_type *target_ptr, DIRECTION *mm, MONSTER_IDX m_idx, bool aware);
