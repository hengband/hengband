#pragma once

#include "system/angband.h"

struct player_type;
void r_visit(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int node, DIRECTION dir, int *visited);
void build_maze_vault(player_type *player_ptr, POSITION x0, POSITION y0, POSITION xsize, POSITION ysize, bool is_vault);
