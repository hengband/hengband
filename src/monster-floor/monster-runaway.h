﻿#pragma once

#include "system/angband.h"

class player_type;
struct turn_flags;
bool runaway_monster(player_type *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx);
