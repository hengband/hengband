#pragma once

#include "util/point-2d.h"

class PlayerType;
void delete_monster_idx(PlayerType *player_ptr, short m_idx);
void wipe_monsters_list(PlayerType *player_ptr);
void delete_monster(PlayerType *player_ptr, const Pos2D &pos);
