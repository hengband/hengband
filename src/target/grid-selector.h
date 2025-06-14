#pragma once

#include "util/point-2d.h"
#include <tl/optional.hpp>

class PlayerType;
tl::optional<Pos2D> point_target(PlayerType *player_ptr);
