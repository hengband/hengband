#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class PlayerType;
void r_visit(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int node, DIRECTION dir, int *visited);
void build_maze_vault(PlayerType *player_ptr, const Pos2D &center, const Pos2DVec &vec, bool is_vault);
