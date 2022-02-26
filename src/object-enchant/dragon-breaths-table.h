#pragma once

#include "effect/attribute-types.h"
#include "object-enchant/tr-types.h"
#include "system/angband.h"

struct dragonbreath_type {
    tr_type flag;
    AttributeType type;
    concptr name;
};

extern const dragonbreath_type dragonbreath_info[];
