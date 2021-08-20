#pragma once

#include "system/angband.h"

typedef struct activation_type {
    concptr flag;
    byte index;
    byte level;
    int32_t value;
    struct {
        int constant;
        DICE_NUMBER dice;
    } timeout;
    concptr desc;
} activation_type;

#define MAX_ACTIVATION_TYPE 138
extern const activation_type activation_info[MAX_ACTIVATION_TYPE];
