﻿#pragma once

#include "system/angband.h"

class player_type;
int32_t turn_real(player_type *player_ptr, int32_t hoge);
void prevent_turn_overflow(player_type* player_ptr);
