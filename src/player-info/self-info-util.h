#pragma once

#include "system/angband.h"
#include "system/object-type-definition.h"

typedef struct self_info_type {
    int line;
    char v_string[8][128];
    char s_string[6][128];
    TrFlags flags;
    char plev_buf[80];
    char buf[2][80];
    concptr info[220];
} self_info_type;

self_info_type *initialize_self_info_type(self_info_type *self_ptr);
