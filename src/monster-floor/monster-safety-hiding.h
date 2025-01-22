#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <optional>

class PlayerType;
std::optional<Pos2DVec> find_safety(PlayerType *player_ptr, short m_idx);
bool find_hiding(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION *yp, POSITION *xp);
