#pragma once

#include "system/angband.h"
#include <vector>

enum random_art_activation_type : uint8_t;
struct activation_type {
    concptr flag;
    random_art_activation_type index;
    byte level;
    int32_t value;
    struct {
        int constant;
        DICE_NUMBER dice;
    } timeout;
    concptr desc;
};

extern const std::vector<activation_type> activation_info;
