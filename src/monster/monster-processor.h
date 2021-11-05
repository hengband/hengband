#pragma once

#include "system/angband.h"

class PlayerType;
void process_monsters(PlayerType *player_ptr);
void process_monster(PlayerType *player_ptr, MONSTER_IDX m_idx);
