#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <vector>

class PlayerType;
enum target_type : uint32_t;
bool target_able(PlayerType *player_ptr, MONSTER_IDX m_idx);
std::vector<Pos2D> target_set_prepare(PlayerType *player_ptr, target_type mode);
void target_sensing_monsters_prepare(PlayerType *player_ptr, std::vector<MONSTER_IDX> &monster_list);
std::vector<MONSTER_IDX> target_pets_prepare(PlayerType *player_ptr);
