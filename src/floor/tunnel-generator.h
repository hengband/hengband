#pragma once

#include "system/angband.h"

typedef struct dt_type dt_type;
bool build_tunnel(player_type *player_ptr, dt_type *dt_ptr, POSITION row1, POSITION col1, POSITION row2, POSITION col2);
bool build_tunnel2(player_type *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int type, int cutoff);
