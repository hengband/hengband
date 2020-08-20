#pragma once

#include "system/angband.h"

int projection_path(player_type *player_ptr, u16b *gp, POSITION range, POSITION y1, POSITION x1, POSITION y2, POSITION x2, BIT_FLAGS flg);
bool projectable(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
int get_max_range(player_type *creature_ptr);
POSITION get_grid_y(u16b grid);
POSITION get_grid_x(u16b grid);
