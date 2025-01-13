#pragma once

#include "util/point-2d.h"

class PlayerType;
void fill_treasure(PlayerType *player_ptr, const Pos2D &top_left, const Pos2D &bottom_right, int difficulty);
