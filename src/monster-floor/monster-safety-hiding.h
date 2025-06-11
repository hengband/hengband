#pragma once

#include "util/point-2d.h"
#include <tl/optional.hpp>

class PlayerType;
tl::optional<Pos2D> find_safety(PlayerType *player_ptr, short m_idx);
tl::optional<Pos2D> find_hiding(PlayerType *player_ptr, short m_idx);
