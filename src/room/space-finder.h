#pragma once

#include "util/point-2d.h"
#include <tl/optional.hpp>

class DungeonData;
class PlayerType;
tl::optional<Pos2D> find_space(PlayerType *player_ptr, DungeonData *dd_ptr, int height, int width);
