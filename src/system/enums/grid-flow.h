#pragma once

#include "util/enum-range.h"

enum class GridFlow : int {
    NORMAL = 0,
    CAN_FLY = 1,
    MAX = 2,
};

constexpr EnumRange<GridFlow> GRID_FLOW_RANGE(GridFlow::NORMAL, GridFlow::MAX);
