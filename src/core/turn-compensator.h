#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
int turn_real(player_type *player_ptr, int hoge);
void prevent_turn_overflow(player_type* player_ptr);
