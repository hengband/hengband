#pragma once

#include "util/point-2d.h"
#include <cstdint>

class PlayerType;
void place_gold(PlayerType *player_ptr, const Pos2D &pos);
void place_object(PlayerType *player_ptr, const Pos2D &pos, uint32_t mode);
