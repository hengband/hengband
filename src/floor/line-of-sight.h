#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
bool los(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
