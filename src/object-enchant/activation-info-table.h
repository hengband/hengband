#pragma once

#include "system/angband.h"

typedef struct activation_type {
    concptr flag;
    byte index;
    byte level;
    s32b value;
    struct {
        int constant;
        DICE_NUMBER dice;
    } timeout;
    concptr desc;
} activation_type;

extern const activation_type activation_info[];
