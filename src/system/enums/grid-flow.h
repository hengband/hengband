#pragma once

#include "util/enum-range.h"

enum flow_type : int {
    FLOW_NORMAL = 0,
    FLOW_CAN_FLY = 1,
    FLOW_MAX = 2,
};

constexpr EnumRange<flow_type> GRID_FLOW_RANGE(FLOW_NORMAL, FLOW_MAX);
