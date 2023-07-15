#pragma once

#include "system/angband.h"

class PlayerType;
void r_visit(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int node, DIRECTION dir, int *visited);
void build_maze_vault(PlayerType *player_ptr, POSITION x0, POSITION y0, POSITION xsize, POSITION ysize, bool is_vault);
