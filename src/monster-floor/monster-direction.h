#pragma once

#include "system/angband.h"

class PlayerType;
bool get_enemy_dir(PlayerType *player_ptr, MONSTER_IDX m_idx, int *mm);
bool decide_monster_movement_direction(PlayerType *player_ptr, DIRECTION *mm, MONSTER_IDX m_idx, bool aware);
