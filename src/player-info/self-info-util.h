#pragma once

#include "system/item-entity.h"

struct self_info_type {
    self_info_type() = default;
    int line = 0;
    char v_string[8][128]{};
    char s_string[6][128]{};
    TrFlags flags{};
    char plev_buf[80]{};
    char buf[2][80]{};
    const char *info[220]{};
};
