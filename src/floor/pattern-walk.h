#pragma once

#include "util/point-2d.h"

class PlayerType;
bool pattern_effect(PlayerType *player_ptr);
bool pattern_seq(PlayerType *player_ptr, const Pos2D &pos);
void pattern_teleport(PlayerType *player_ptr);
