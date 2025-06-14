#pragma once

#include "util/point-2d.h"
#include <cstdint>
#include <tl/optional.hpp>

extern uint8_t display_autopick;

class DisplaySymbolPair;
class PlayerType;
DisplaySymbolPair map_info(PlayerType *player_ptr, const Pos2D &pos);
tl::optional<uint8_t> get_monochrome_display_color(PlayerType *player_ptr);
