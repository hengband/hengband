#pragma once

#include "util/point-2d.h"
#include "view/colored-char.h"
#include <cstdint>
#include <optional>

extern uint8_t display_autopick;

class PlayerType;
ColoredCharPair map_info(PlayerType *player_ptr, const Pos2D &pos);
std::optional<uint8_t> get_monochrome_display_color(PlayerType *player_ptr);
