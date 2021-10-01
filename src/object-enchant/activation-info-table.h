#pragma once

#include "system/angband.h"
#include <vector>

struct activation_type {
    concptr flag;
    byte index;
    byte level;
    int32_t value;
    struct {
        int constant;
        DICE_NUMBER dice;
    } timeout;
    concptr desc;
};

extern const std::vector<activation_type> activation_info;
