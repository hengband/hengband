#pragma once

#include "system/angband.h"
#include "object-enchant/tr-types.h"
#include "effect/attribute-types.h"

struct dragonbreath_type {
    tr_type flag;
    AttributeType type;
    concptr name;
};

extern const dragonbreath_type dragonbreath_info[];
