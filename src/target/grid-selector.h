#pragma once

#include "util/point-2d.h"
#include <optional>

class PlayerType;
std::optional<Pos2D> point_target(PlayerType *player_ptr);
