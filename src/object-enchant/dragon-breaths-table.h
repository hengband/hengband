#pragma once

#include "system/angband.h"
#include "object-enchant/tr-types.h"
#include "effect/attribute-types.h"

typedef struct {
    tr_type flag;
    AttributeType type;
    concptr name;
} dragonbreath_type;

extern const dragonbreath_type dragonbreath_info[];
