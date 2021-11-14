#pragma once

#include <vector>

#include "system/angband.h"

class PlayerType;
bool target_able(PlayerType *player_ptr, MONSTER_IDX m_idx);
void target_set_prepare(PlayerType *player_ptr, std::vector<POSITION> &ys, std::vector<POSITION> &xs, BIT_FLAGS mode);
void target_sensing_monsters_prepare(PlayerType *player_ptr, std::vector<MONSTER_IDX> &monster_list);
