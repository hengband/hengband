#pragma once

#include "system/angband.h"

class PlayerType;
struct turn_flags;
bool runaway_monster(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx);
