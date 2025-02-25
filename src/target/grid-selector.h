#pragma once

#include "util/point-2d.h"
#include <optional>

class PlayerType;
std::optional<Pos2D> tgt_pt(PlayerType *player_ptr);
