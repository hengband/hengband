#pragma once

#include "autopick/autopick-methods-table.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include <cstdint>
#include <tl/optional.hpp>

extern EnumClassFlagGroup<AutopickMethod> display_autopick;

class DisplaySymbolPair;
class FloorType;
class PlayerType;
bool is_revealed_wall(const FloorType &floor, const Pos2D &pos);
DisplaySymbolPair map_info(PlayerType *player_ptr, const Pos2D &pos);
tl::optional<uint8_t> get_monochrome_display_color(PlayerType *player_ptr);
