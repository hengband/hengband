#pragma once

#include "system/angband.h"
#include "object-enchant/tr-types.h"

typedef struct {
    tr_type flag;
    int type;
    concptr name;
} dragonbreath_type;

extern const dragonbreath_type dragonbreath_info[];
