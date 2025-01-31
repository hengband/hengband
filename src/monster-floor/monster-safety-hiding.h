#pragma once

#include "util/point-2d.h"
#include <optional>

class PlayerType;
std::optional<Pos2DVec> find_safety(PlayerType *player_ptr, short m_idx);
std::optional<Pos2D> find_hiding(PlayerType *player_ptr, short m_idx);
