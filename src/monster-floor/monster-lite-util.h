#pragma once

#include "util/point-2d.h"
#include <cstdint>

class MonsterEntity;
struct monster_lite_type {
    monster_lite_type(uint32_t grid_info, const MonsterEntity &monster);
    bool mon_invis;
    Pos2D m_pos;
};
