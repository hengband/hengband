﻿#pragma once

#include "system/angband.h"

struct monster_type;
class player_type;
bool common_saving_throw_control(player_type *player_ptr, HIT_POINT pow, monster_type *m_ptr);
bool common_saving_throw_charm(player_type *player_ptr, HIT_POINT pow, monster_type *m_ptr);
PERCENTAGE beam_chance(player_type *player_ptr);
