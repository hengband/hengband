#pragma once

#include <vector>

#include "system/angband.h"

struct player_type;
bool target_able(player_type *player_ptr, MONSTER_IDX m_idx);
void target_set_prepare(player_type *player_ptr, std::vector<POSITION> &ys, std::vector<POSITION> &xs, BIT_FLAGS mode);
void target_sensing_monsters_prepare(player_type *player_ptr, std::vector<MONSTER_IDX> &monster_list);
