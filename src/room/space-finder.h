#pragma once

#include "util/point-2d.h"
#include <optional>

class DungeonData;
class PlayerType;
std::optional<Pos2D> find_space(PlayerType *player_ptr, DungeonData *dd_ptr, int height, int width);
