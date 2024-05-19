#pragma once

#include "util/point-2d.h"
#include "view/colored-char.h"
#include <cstdint>

extern uint8_t display_autopick;

class PlayerType;
ColoredCharPair map_info(PlayerType *player_ptr, const Pos2D &pos);
